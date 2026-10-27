# Working-surface reconstruction: architecture review

Current decision record, 2026-10-27. DC-17/DC-18 remain open. No accepted M6
adaptation/transfer/restart workflow exists. This review precedes further
implementation following the user's latest instruction.

## Demonstrated failure and current code state

Build36 serial campaign `staged57/cases/serial_full_20261027T153436_2673844`
finished in99.7804s with unchanged pins:226/228 cases passed,1129639/1129642
assertions. The overlapping-star test exhausts arithmetic work on event5 (the
sixth move), using the existing per-event reservation. The curved Euler and RANS
runtime variants also exhaust67,207,168 arithmetic work after29 private edits.
The original CFD state is retained. Neither failure is a successful adaptation.

The recurrence is structural: a transaction flattens a reference region using
current vertices and retained traces; clipping produces new edge paths; a
neighboring transaction imports those paths as boundary constraints. Discarding
face-interior cuts does not remove that recurrence. The earlier same-star
oscillation keeps its perimeter fixed and misses it.

At the time of this review the connected prototype was frozen unbuilt; build37
subsequently compiled it. Build39 passed 53 focused cases/8247 assertions, and
build40's 16-event neighboring-star cost case met its phase bounds. The full mapped
runtime still fails, so this is narrow fixture qualification. Its independent
original-star/strip path selector is custom and
does not establish joint network admissibility. It must not become the production
strategy merely because its primitive tests pass. Latest pushed source checkpoint
before the compatibility results: `a61e28bf58`, explicitly WIP.

## Established methods versus repository choices

| Mechanism | Established part | Repository-specific choice and evidence |
|---|---|---|
| Local adaptation | Split/collapse/swap, constrained relocation, cavity validity | Native proposal enumeration, staged scheduler, resource accounting and SU2 transaction integration are custom. Operator availability alone does not qualify the algorithm. |
| Surface parameterization | Tutte/positive-weight harmonic embedding with the required topology and boundary assumptions | Re-solving a chart from current subdivisions, then making its clipped edge paths persistent constraints, is our choice. A parameterization theorem does not bound this recurrence. |
| Patch reconstruction | Constrained planar graphs, triangulation, oriented-chain checks | The native exact corridor recovery, coverage encoding and interface traces are custom implementations. Their exact arithmetic does not make the representation history independent. |
| Geometry association | Fixed model entities, point/curve/surface parameters, constrained projection | Original cubic controls and `SurfaceBinding` provide this foundation. Persisted recursive covers/traces have additionally become construction authority. That coupling is the blocker. |
| Parallel scheduling | Conflict graphs, independent cavities, ownership/migration and deterministic ties | Native Directory reservations/publication are custom glue. Construction must keep its validity-only priorities; quality-ranked examples cannot be copied unchanged. |
| Verification/transfer | Geometric predicates, intersection-based conservative transfer, independent coverage checks | Native whole-region certificates, BL constraints, BDF history and restart are SU2 obligations. They remain mandatory after any library reuse. |

CGAL documents Tutte/mean-value parameterizations and their topology, convex-border
and solver assumptions; it also supplies seam meshes. These are established
building blocks, not a guarantee for independently re-charted neighboring edits.
[CGAL parameterization manual](https://doc.cgal.org/latest/Surface_mesh_parameterization/index.html).

## Comparison with existing implementations

**Gmsh: model entities and reusable parameterizations.** Mesh nodes carry their
geometric entity; edge and face nodes additionally carry curve/surface parameters.
Discrete geometry is classified into surfaces, curves and points and parameterized
before remeshing. This supports separating fixed geometry from the working mesh.
The reusable principle is entity/parameter association, not adoption of Gmsh's
entire solver/mesher. Its discrete STL geometry is not an exact smooth CAD model.
[Node representation](https://raw.githubusercontent.com/live-clones/gmsh/master/src/geo/MVertex.h),
[discrete remeshing and CAD distinction](https://www.gmsh.info/doc/texinfo/gmsh.html#t13).

**CGAL: remesh a patch, then project to the reference.** The remesher sequences
splits, collapses, flips, tangential relaxation and projection to the initial
surface; patch maps, constrained edges and projection callbacks are explicit.
This is a useful separation of working topology and geometry authority. Its
default isotropic sizing and constraint policies are not the SU2 anisotropic/BL
contract. Its documentation also warns that oversized protected constraints can
cause cascading insertions: constraints and sizing must be coordinated, not
rescued by ever-larger work limits.
[CGAL remeshing API](https://doc.cgal.org/latest/PMP_Remeshing/group__PMP__local__remeshing__grp.html).

**Mmg: local geometric reconstruction and anisotropic remeshing.** The public
Bezier implementation constructs edge controls from endpoint positions,
normals/tangents and ridge tags. This is relevant to the existing cubic backend;
it does not establish an immutable original-geometry association across repeated
SU2 events. SU2 already has an MMG adapter and MMG headers are installed locally.
The inspected public interface supplies meshes, metrics, references and numerical
parameters; no native cubic-oracle/association callback was identified there.
Reusing its complete remesher would not implement the requested native distributed
workflow. Keep it as an independent comparator; do not silently refit the authority
from each new working mesh.
[Mmg Bezier source](https://raw.githubusercontent.com/MmgTools/mmg/master/src/common/bezier.c),
[Mmg project](https://github.com/MmgTools/mmg).

**Omega_h: a coherent adaptation schedule and parallel cavities.** Its source
separates length satisfaction, quality repair/snapping and conservation correction.
Its independent-set implementation synchronizes conflicting candidates; a concrete
comparator uses quality and global-number ties. Reuse the ownership/conflict design
and staged organization, while retaining SU2's explicit validity-first construction
and final gates. It is not evidence that our current cavity ordering converges.
[Adaptation source](https://raw.githubusercontent.com/sandialabs/omega_h/main/src/Omega_h_adapt.cpp),
[independent sets](https://raw.githubusercontent.com/sandialabs/omega_h/main/src/Omega_h_indset_inline.hpp),
[comparator](https://raw.githubusercontent.com/sandialabs/omega_h/main/src/Omega_h_indset.cpp).

**Geometry Central: fixed input, mutable intrinsic triangulation, disposable overlay.**
The input mesh/geometry stay unchanged; current vertices have input-surface
locations. Mutation invalidates the derived common subdivision, which can be
regenerated. This is the most directly relevant representation comparison.
Its path-network shortening preserves the absence of new crossings and provides
a conforming intrinsic triangulation, unlike an uncoordinated choice of one path
per edge. These are reasons to test practical reuse before completing another
custom path engine.
[Intrinsic representation](https://geometry-central.net/surface/intrinsic_triangulations/basics/),
[network algorithm](https://geometry-central.net/surface/algorithms/flip_geodesics/).

Important fit questions remain. Signpost tracing has documented floating-point
robustness limits. Integer coordinates preserve common-subdivision topology, but
documented API gaps include input-to-intrinsic point location; default mollification
also changes intrinsic lengths. The common-subdivision API requires input vertices
to remain in the background triangulation. Those can be virtual geometry vertices,
not fixed CFD nodes, but the adapter must demonstrate that separation. None of
these APIs supplies native cubic certificates, volume BL reconstruction, MPI
publication or SU2 restart by itself.
[Signpost limits](https://geometry-central.net/surface/intrinsic_triangulations/signposts/),
[integer coordinates](https://geometry-central.net/surface/intrinsic_triangulations/integer_coordinates/),
[subdivision contract](https://geometry-central.net/surface/intrinsic_triangulations/common_subdivision/),
[mutator interface](https://raw.githubusercontent.com/nmwsharp/geometry-central/master/include/geometrycentral/surface/intrinsic_triangulation.h).

Geometry Central is MIT licensed and can use an existing Eigen installation;
SuiteSparse is optional. It is not currently an SU2 dependency. A bounded local
compatibility probe is warranted; production integration is not assumed.
[License](https://raw.githubusercontent.com/nmwsharp/geometry-central/master/LICENSE),
[dependencies](https://geometry-central.net/build/dependencies/).

## Connected design to implement

1. Keep geometry authority immutable. Persistent associations identify the body,
   physical entity/feature and point parameters. Separate true features from chart
   seams. Preserve current boundary mobility and body separation.
2. Assemble one coordinated working-surface patch with the complete incident-face
   and volume-owner closure of every changed edge. A surviving path may change
   only with all incident faces reserved and reconstructed. Legacy noncanonical
   interfaces require coordinated refresh, not recursive import as new chart data.
3. Rebuild the affected network against original geometry and current associations.
   Paths must be planned jointly or have a certified common embedding. Do not use
   independently chosen per-edge charts as an admissibility guarantee. Geometry
   intersections/subdivisions are derived output, never the next event's authority.
4. Reconstruct all affected faces from that shared network, then construct valid
   volume replacements. Discard obsolete interior geometry knots. Any numerical
   surface parameterization only proposes a map; native exact validity, geometry,
   feature/contact and whole-region certificates decide admission.
5. Publish geometry associations, shared-edge contracts, affected face covers and
   volume rows atomically through the existing Directory. Source reconstruction
   and restart must consume the same accepted association generation. Retired
   descriptors and caches are removed. Recompute disposable overlays from fixed
   geometry/current state rather than serializing a parent-subdivision chain.
6. Follow with metric/BL-aware improvement and all unchanged final acceptance,
   conservative transfer/history and restart checks.

The representation bound must depend on current reference-region size, current
mesh entities, current endpoint precision and actual current geometric
intersections. It must not depend on how many previous adaptations generated the
state. This is a design obligation, not a claim already proved by the WIP code.

### Candidate representation and cost argument

For an ordinary internal edge the proposed path is a function of the immutable
original charts, fixed body/feature policy and two current associations. It does
not import the prior face subdivision or prior internal path. One original star
or a deterministically selected original-facet strip supplies the proposal; its
piece count cannot exceed that chart's original facets. This choice is custom.
It proposes a route; it does not prove that the complete prescribed graph is
routable by those routes. The collective embedding certificate is mandatory.

All incident candidate faces use the same proposed contract. A patch perimeter
keeps its accepted contract because its other incident face is outside the
transaction. When an edge becomes internal to a subsequent neighboring patch,
both incident faces participate and its path is rebuilt against original geometry.
New interface paths are never formed by appending clipped subdivisions to an
old path. Genuine curves retain their topological arc but merge obsolete knots
on the same original edge; they do not become artificial fixed CFD boundaries.

The fresh PSLG is seeded by the accepted region's geometric perimeter, current
associations and current proposed edge paths. Old face-interior knots are omitted.
For an admitted noncrossing network, reference triangulation size is controlled
by those current constraints and the original reference region. Additional chord
points are interior geometry correspondence, not new CFD points or future path
inputs. The sparse solve produces dyadic proposal coordinates; exact positivity,
physical cubic orientation/contact and region/feature certificates remain separate.
Current input precision and geometric intersections still affect arithmetic cost.
This argument removes the specific historical recurrence; it is not a universal
complexity or robustness proof. Legacy interface descriptors can still cost work
until coordinated replacement, and arbitrary proposed networks may be refused.
Save measured counts/work/bytes/time across repeated complete adaptations before
qualifying bounded production cost.

The immediate reuse decision is to use the existing Eigen dependency for sparse
linear algebra if a face-interior solve remains necessary, instead of adding a
second custom CG implementation. Eigen exposes sparse CG with explicit iteration
and residual controls. The native exact acceptance certificate still applies.
[Eigen CG](https://libeigen.gitlab.io/eigen/docs-nightly/classEigen_1_1ConjugateGradient.html).

The surface-network implementation assessment uses a bounded Geometry Central
fit probe: current points including moved/coarsened original CFD nodes, genuine
features/nonseparating arcs, input-to-current association, disposable subdivision,
and repeated neighboring edits. Disable geometry-changing mollification for that
comparison. Demonstrate current-local-patch cost rather than rebuilding a whole
body for every local edit. If an API is unsuitable, record the concrete missing
capability; lack of an existing dependency alone is not a reason to rewrite it.

The current candidate enforces owner closure collectively: every supplied face
cover must have its owning volume row among the replacement rows, and a retained
face must also have its accepted owner among the removals. Existing Directory
version advancement and change notifications then apply; no metadata-only bypass
or separate cache epoch is introduced. Exact reference-network certification is
separate from physical cubic embedding/contact checks, which remain mandatory.

Custom machinery remains necessary for the cubic geometry oracle and certificates,
association adapters, resource admission, ownership/atomic publication and SU2
volume/BL/transfer/restart semantics. This does not justify a custom replacement
for every mature geometric or linear-algebra primitive.

## Callers, cache locality and persistence

The connected change reaches `PrepareCurvedAtlas`, movement proposals, shared-path
planning and face reconstruction; the common boundary-operator completion path;
optimizer assembly; Directory dependency/reservation/publication; boundary
parameter state and reference I/O; source-envelope construction; restart/rebinding.

Current inspection found cell-version dependency checks in
`CNativeDistributed3D.cpp`, engine cache drops for removed cells, and changed-region
coverage validation in `BoundaryParameterState::Prepare`. The full accepted-cover
audit occurs at state construction, not every local publication. Global parameter
revision protects private-view epochs. Do not invent another cache/version layer
without evidence that existing cell/association invalidation is insufficient.

The geometry interface remains backend neutral: entity/surface/curve evaluation,
scoped projection, normals/curvature, feature/body identity, geometric error and
persistent association. Current cubic controls remain authoritative. Future OCCT
must use model topology and surface/curve parameters; STEP first, then faceted STL,
then IGES. No claim of exact smooth CAD from STL and no importer implementation
in this change.
[OCCT modeling data](https://dev.opencascade.org/doc/overview/html/occt_user_guides__modeling_data.html),
[STEP translator](https://dev.opencascade.org/doc/overview/html/occt_user_guides__step.html).

## Reuse probe provenance and open fit questions

The isolated candidate source is Geometry Central commit
`019669ddabda05e0f71fa3587cfb3c1dadf19cb8`, with happly
`88f79725d32d78e0e637ebf42d8432d519e1c7d0`, using SU2's existing Eigen.
Evidence lives under `staged57/reference_network_probe_v1`. Production dependencies
and the accepted geometry model have not changed.

The first library build passed314.49s with1663 unchanged pins; largest sampled
owned process tree was1,054,257,152 bytes (not a whole-job peak). Run1 failed after
one event. Its adapter supplied an edge point as a face insertion, allowing a
zero-area background triangle. Preserve that failure; correct the support kind
and documented geometry refresh before assessing the library. Background
compression also requires slot labels that survive index changes.

Source inspection confirms integer input-to-intrinsic location throws an
unimplemented exception and removal refuses original/fixed-edge vertices.
Reference vertices may remain virtual rather than CFD nodes, but movable feature
samples need a clean local rebuild or a demonstrated supported operation; keeping
retired samples indefinitely is unacceptable. No native cubic, joint-network,
volume/BL, MPI or transfer qualification follows from this probe.

Build4 explicitly invalidates the derived common subdivision after each adapter
mutation: pinned `splitFace()` omits that invalidation, while refreshing numerical
quantities alone leaves the cache stale. Planar3 and m63 preserved the resulting
count/path failure; no upstream source was changed and the checks were retained.
With the adapter correction, planar4 and m64 each complete all16 neighboring
insert/remove events with unchanged pins. Current vertices/faces/capacities remain
constant; overlay counts settle after the initial events. The complete measurements
and memory-sampling limits are in
[probe summary](../integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57/reference_network_probe_v1/probe_summary.md).
The probe does not count native exact arithmetic work.

**Fit decision:** do not integrate Geometry Central as a complete native network
engine. `FlipEdgeNetwork` owns a signpost triangulation and requires already
embedded input halfedge paths; `addPath()` explicitly requires an unobstructed
path. Its integer-coordinate triangulation cannot directly recover an arbitrary
prescribed CFD graph from associations. Virtual reference vertices remain in its
background topology. The missing capability is joint CFD graph recovery with
cyclic order and movable genuine curves. The successful probe establishes the
background/overlay separation only. Adding the dependency would leave the central
network-recovery work custom, alongside extra association and retirement adapters.

The next connected candidate therefore reuses the existing exact reference PSLG
builder and region/publication machinery, with Eigen replacing custom linear
algebra. Canonical edge paths are **proposals**, never admissibility evidence.
Admission must construct one shared constrained reference mesh for the complete
candidate network, establish disjoint face disks, strictly positive maps with
identical shared-edge parameters, and exact replacement of the old region. This
collective certificate, rather than independent path selection, must establish a
common embedding. If it cannot admit the overlapping-neighbor sequence and the
difficult runtime/M6 target, reject the candidate and reconsider joint topology
reconstruction; do not add alternate path selectors or increase budgets. This
candidate is now qualified only by build39's focused53 cases/8247 assertions,
including the16-event overlapping-star failure and atomic crossed-network/owner
rejection. Six-rank M6 was interrupted by a diagnostic debugger STOP confound; a fresh
unattached run is required and no native MPI defect is established. This is not a proven general
graph-routing algorithm or a completed adaptation/transfer/restart workflow.

A complete six-face M6 wing star is extracted using the existing mesh parser,
with input/parser/extractor hashes and original IDs saved. Center ID4210 has a
selected30.89-degree radial turn. This is faceted reference-patch data for the
reuse probe, not a substitute for the cubic M6 geometry or full M6 acceptance.

Any rebase of face/edge association must occur after freezing the donor geometry,
inside the private adaptation transaction, and participate in final conservative
transfer. Metadata changes can redistribute curved cell envelopes even when flat
node coordinates are unchanged. Restart must reconstruct the accepted generation,
not silently reinterpret old donor geometry with a new association rule.

Archive admission must also be assessed using actual record counts and scalar
widths. The existing archive arithmetic/storage envelope derives integer capacity
from whole encoded byte length; it can become falsely prohibitive for a large
otherwise compact state. Do not increase that envelope indefinitely or declare
persistence qualified without exercising the actual accepted mesh.

## Acceptance and evidence

- Verify the compiled filters select all intended reconstruction and workflow
  cases. Preserve failed receipts and unchanged source/binary/input/helper pins.
- Run the failing overlapping neighboring-star sequence and repeated complete
  adaptation events. Record per-event current nodes/faces/edges, reference and
  coverage pieces, trace knots, rational widths, arithmetic work, sampled memory
  and runtime. Repeat endpoint states and include moving neighboring interfaces.
- Exercise actual split/collapse/swap/movement, full incident-face publication,
  distributed rollback and different metric/geometry fields. Primitive success
  establishes only its stated scope.
- Use M6 early after the connected representation works; retain accepted/private
  status separately. Then qualify Euler and requested BL/RANS cases, publication,
  conservative transfer/BDF history, persistence/reload and restart.
- Final gates remain q>=.20, J>=.05, metric edge<=1.8, requested first height within
  10%, prescribed geometry tolerance/features/bodies and500k metric complexity.
  No budget increase, quality-gate weakening, CFD-mode dispatch or alternate local
  fallback is part of resolving this blocker.

The separate unchanged implicit-workflow failure is now attributed to generic single-triangle verification; see [measured cost and connected certificate design](GEOMETRY_VERIFICATION_COST.md). This does not close the explicit-network or accepted-M6 obligations.
