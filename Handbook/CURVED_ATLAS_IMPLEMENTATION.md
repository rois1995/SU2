# Bounded curved atlas: connected implementation design

Implementation contract, 2026-10-27; connected C++ work is underway, not qualified. This expands
[the mobility diagnosis](CURVED_BOUNDARY_MOBILITY.md). The missing capability is
redistribution across artificial original cubic-chart seams, with genuine
features, bodies, carriers, requested heights, transfer and restart preserved.
Quality-stage ordering does not supply this capability. Build19's early capture
replaced seven generated physical points; all seven retained their dimension-one
original-edge feature identities. These are on-chart edge slides, not released
original reference vertices or demonstrated cross-chart freedom/M6 closure.

## Current architectural decision: remove historical interior refinement

User direction, 2026-10-27: arithmetic improvements alone do not close growth.
The next implementation reconstructs a clean local working reference surface from
immutable original charts, current forward bindings and required perimeter
traces. Accepted coverage supplies its assigned reference region and boundary;
its accumulated interior triangulation is discarded. Exact-edge cancellation is
a certificate cost improvement, not a descriptor-growth solution.

Acceptance must track piece/trace counts, rational sizes, actual work, bytes and
transport through repeated and adjacent transactions, with immutable geometry,
features, bodies, source/transfer/restart and final mesh gates preserved. Bounds
must follow current constrained-region complexity, not event count. Retained
interface traces cannot hide unbounded history: their simplification requires
coordinated replacement of all incident faces or a reconstructed region that
retires them. Qualify the saved difficult M6 support and full workflow; do not
substitute passing primitive cases or larger budgets.

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
2. Extract the accepted reference region's boundary, canceling internal pieces.
   Build a fresh constrained triangulation from original facet triangles, current
   associations and required boundary knots. Original facets are seeds, not a
   convex-hull replacement for missing regions: exact winding keeps per-facet
   holes and disconnected portions. Cut each local physical star at genuine
   feature paths even when the global smooth-sheet graph remains connected.
   Each glued local atlas domain still needs certified disk topology.
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
5. Retriangulate the admitted sheet domain with unchanged external interfaces
   and physical constraints. Shared paths inside the admitted region may be
   reconstructed only with all incident faces participating. Intersect new active
   triangles with the freshly rebuilt reference cells using rational clipping. Each intersection yields both original parameters
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
The later build32 M6 diagnostic result is recorded below; acceptance remains open.

Difficult M6 build28 diagnostic is terminal FAIL178.39s, unchanged pins
(`staged57/cases/m6_stage_20261027T121816_2120374`): policy derivation reports674
protected edges,2corners,671curve points and23806free surface points. Activation
then exhausts its arithmetic budget re-auditing an already validated surface in
the general constructor, before mesh construction starts. This is setup cost,
not evidence of invalid mesh construction or failed smoothing. The connected
correction migrates policy on a bounded copy of validated accepted state, verifies
policy/physical corners, and preserves coordinates/bindings/coverage/version/
revision. No bypass option is added to the untrusted constructor or decoders.
Caps stay unchanged; migration-equivalence and rollback tests are added.

Selection correction: the first focused runs selected NativeCurvedAtlas3D from
the geometry tests, while the new remapper cases used NativeCurvedAtlasRemap3D.
They therefore did not qualify the previously stated motion/second-event paths.
The remapper cases now also carry the selected common tag; they and the new
migration test must pass before claiming that coverage. Earlier counts25/4261
remain valid for the actually selected geometry/publication/archive/source checks.

Build29 PASS1108.27s/944 unchanged pins. Corrected focused selection
`staged57/cases/atlas_checks_20261027T124528_2195618` is terminal:28/29 cases,
4317/4318 assertions,14.68s, unchanged pins. Policy migration, first original-
vertex remap and shared curve-sample motion pass. Second accepted remap exhausts
its536M arithmetic work in ValidateCoverageReplacement->SplitAtlasEdges, proved
by `remap_second_event_debug.log`. This identifies excessive pairwise work on
microtriangle interior edges. The correction algebraically cancels exact directed
edges first, then subdivides/checks the residual reference boundary. The same
positive2-chain equality proof and resource limits remain. A separate direct
MoveVertex/tetrahedron-star/second-accepted-state case is also added to check
caller wiring. No M6 mesh or full transfer qualification is claimed.

Connected reconstruction implementation is frozen for build30/session54900:
950 pins, one low-priority worker. The new reference PSLG/corridor triangulator
seeds no historical interior nodes; policy activation reuses validated state;
shared-path metadata transactions require complete incidence; local feature arcs
are cut independently of global connectivity; retired/coordinated curve paths
use original-edge coordinates. Protected reference curves must map to an active
face boundary. A small native backend adapter supplies real evaluation,
association/support and bounded geometry queries, while PN/source certificates
stay specialized. New operator, repeated-event, nonseparating-feature, region
history-equivalence, coordinated-publication and backend cases are selected.
The source-tag audit found all14 cases in the three new test files selected;
runtime listing and results remain pending. No M6 acceptance is claimed.

Build30 terminal compiler failure1180.22s/950 unchanged pins: production code
compiled, new reference tests used an ambiguous Point helper. Terra renamed it;
build31 PASS24.07s. The first verified45-case run failed7 assertions: six proposal
samples used interior-only bindings at original vertices/edges, and one assertion
expected an internal diagnostic hidden by the public API. Canonical support
selection and the public-message assertion corrected these without changing gates.
Build32 PASS78.14s/950 unchanged pins. Focused
`staged57/cases/atlas_checks_20261027T144415_2532842` PASS38.79s,45 cases/7561
assertions, pins unchanged. Compiled test listing/filter audit confirms all14
intended cases in the three new files selected. Actual MoveVertex/repeated state,
16-event descriptor stability, nonseparating curve motion, protected-curve burial
refusal, history-equivalent reference triangulation/holes, coordinated metadata,
backend differential/projection and source/archive cases pass. These remain
focused connected evidence, not accepted M6/whole workflow. Six-rank M6 build32/session90654 TERMINAL FAIL54.3039s, pins unchanged; largest sampled individual RSS392314880 bytes. Setup work174193 passed, but import rejected “Native3D physical triangle leaves its immutable oriented marker facets.” before meshing. Evidence: `staged57/cases/m6_stage_20261027T144634_2539145`. Full accepted mesh, broad serial/MPI, adaptation, transfer and restart remain unqualified.


Build33 addresses the M6 import caller mismatch: active mobility routed even exact
accepted faces through a required transaction reservation, absent in read-only
import. Recognize only identical accepted oriented topology on a clean, nonprivate
parameter view; local changes/proposals retain the existing certification path.
The policy-migration regression now checks accepted lookup with no reservation and
refuses changed coordinates, reversed orientation and a different marker. Build33
passed57.15s/950 unchanged pins. Focused atlas checks
`cases/atlas_checks_20261027T145320_2558161` pass38.85s/45 cases/7610 assertions
with unchanged pins. M6 retry/session31910 TERMINAL REJECT190.56s with unchanged pins. Setup/import
and source preparation passed,814 private edits published; then trace-only
metadata incorrectly invoked a nonempty reference replacement proof. Native
reason “Empty reference coverage replacement.” Original CFD state retained;
rejected-exit shards exist, quality-entry shards do not, wrapper rejects. Build34
skips the region proof only if both old/new face lists are empty, retaining
surviving-edge/equal-trace/complete-incident checks and unused-point rejection.
Focused regression admits inherited trace-only metadata and refuses changed
trace-only paths atomically. Expanded broad filters add previously omitted
runtime/history/veto and barycentric workflow checks. No new acceptance implied.

Build34/session16636 PASS87.15s/950 unchanged pins. Compiled-filter audit
`build34_{serial_full,mpi6}_filter_audit.json` verifies all30 intended reconstruction
and added workflow cases selected (227 serial/204 MPI). Expanded serial
session69002 is live; source/helpers frozen until terminal.

## Current integration work

The [working-surface reconstruction decision](WORKING_SURFACE_RECONSTRUCTION_DECISION.md) calls for a mature-backend compatibility probe before implementation resumes. Geometry Central on fixed input with a disposable overlay is a candidate, not a selected production backend; vendored Eigen is the practical linear-algebra reuse candidate instead of custom CG. Independent per-edge chart work is not the selected joint-network production design. Canonical trace, trace coverage and integration changes are unbuilt, unvalidated WIP and frozen. No jobs are live. Native geometry authority and all final gates remain unchanged.

Build35 and build36 passed with 950 unchanged pins. Build36 serial_full/session68944 TERMINAL FAIL99.7804s, pins unchanged: 226/228 cases and 1129639/1129642 assertions passed; largest sampled individual RSS145432576 bytes. Evidence: `staged57/cases/serial_full_20261027T153436_2673844`. The overlapping-star test exhausted atlas work on event 5 (sixth move); curved Euler/RANS runtime again exhausted 67207168 work after 29 accepted private edits and 35 attempts. Retained path history is an architectural blockage not resolved by discarding interior cuts. Develop a connected strategy before MPI or M6 retry; accepted mesh, transfer and restart remain unqualified.

Finish uses direct immutable-chart ExportParameters and CheckSurface only when
patch, private-view and accepted face/edge correspondence is implicit and candidate
bindings support the anchors. Explicit or cross-chart correspondence selects fresh
PL reconstruction upfront, without a failure fallback. Prepare retains whole-union,
trace and incident-face proofs. Same-star oscillation alone does not establish
bounded cost for moving neighbors; the overlapping-star failure demonstrates the
remaining limitation. Existing predicate and repeated-event resource tests remain.


## Connected reconstruction candidate after mature-design review (2026-10-27)

See [architecture decision](WORKING_SURFACE_RECONSTRUCTION_DECISION.md). The
Geometry Central fixed-input/disposable-overlay probe completes16 moves on flat
and M6 faceted stars, but does not recover the prescribed CFD network or qualify
movable features. It is not being added as a production dependency.

The native candidate builds one constrained reference complex for all proposed
shared paths, labels disjoint candidate face disks and certifies positive maps,
consistent edge parameters and exact region replacement. Independent canonical
paths are proposals only. Eigen replaces custom PCG using the existing vendored
headers, bounded iterations/work/storage, finite and true-residual checks.
Build37 passes78.18s/1293 unchanged pins. Build38 adds collective volume-owner
closure for association publication and an atomic missing-owner rejection test;
validation is in progress. Source donor geometry is frozen before private edits;
reader/rebind and sidecar persistence use the accepted generation after transfer.
No accepted M6 mesh or native workflow qualification is claimed.


Build38 passed111.93s/1293 unchanged pins. Its focused run
`staged57/cases/atlas_checks_20261027T172708_3041237` failed8/53 cases,
7799/7807 assertions,24.94s, unchanged pins. Seven failures shared a reference
builder bug: boundary lists were compared with mandatory knots, although new
knots can subdivide the same geometric chain. Final comparison now uses the
existing collinear-chain canonicalizer without knot retention on both sides;
mandatory-point/trace inclusion and all exact region/embedding checks remain.
The eighth failure was a new fixture's reversed order of two cubic controls.

Build39 passed75.18s/1293 unchanged pins. Focused run
`staged57/cases/atlas_checks_20261027T173019_3050687` passes all53 cases/8247
assertions in47.83s, pins unchanged. This includes the previously failing16-event
overlapping-star sequence with per-phase descriptor/width/work/storage bounds.
The collective network counterexample and owner-publication rejection pass.
These remain fixture/connected-check scope; six-rank M6 diagnostic is now live
at `staged57/cases/m6_stage_20261027T173120_3053606`, session96782. No accepted
mesh or full transfer/restart qualification follows from the focused pass.
