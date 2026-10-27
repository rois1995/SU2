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

The working tree contains an **unbuilt, unvalidated prototype** separating fixed
reference edge planning from prescribed-trace face reconstruction. It is frozen
for this review. Its independent original-star/strip path selector is custom and
does not establish joint network admissibility. It must not become the production
strategy merely because its primitive tests pass. Latest pushed source checkpoint
before this review: `bf3c4e94a6`, explicitly WIP.

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

The immediate reuse decision is to use the existing Eigen dependency for sparse
linear algebra if a face-interior solve remains necessary, instead of adding a
second custom CG implementation. Eigen exposes sparse CG with explicit iteration
and residual controls. The native exact acceptance certificate still applies.
[Eigen CG](https://libeigen.gitlab.io/eigen/docs-nightly/classEigen_1_1ConjugateGradient.html).

The surface-network implementation decision follows a bounded Geometry Central
fit probe: current points including moved/coarsened original CFD nodes, genuine
features/nonseparating arcs, input-to-current association, disposable subdivision,
and repeated neighboring edits. Disable geometry-changing mollification for that
comparison. Demonstrate current-local-patch cost rather than rebuilding a whole
body for every local edit. If an API is unsuitable, record the concrete missing
capability; lack of an existing dependency alone is not a reason to rewrite it.

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
