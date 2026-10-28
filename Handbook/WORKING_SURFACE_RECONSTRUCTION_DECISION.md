# Working-surface reconstruction: architecture review

Current decision record, 2026-10-27. DC-17/DC-18 remain open. No accepted M6
adaptation/transfer/restart workflow exists. This review precedes further
implementation following the user's latest instruction.

## Requirements review supersedes earlier attributions

The [2026-10-28 requirements/design review](ADAPTATION_REQUIREMENTS_REVIEW.md)
separates explicit requirements, inherited numerical settings and custom mechanisms.
Its connected fixed-model/current-patch design supersedes treating exact chart
ownership, persistent smooth paths or budget formulas as user requirements.
The fresh-work split now succeeds; no geometric impossibility is established.
Full replacement and accepted workflow remain pending. Historical plans below
are not additional user constraints.

## Current architectural finding, 2026-10-28

The historical shared-edge recurrence below was real in build36; it is not an
identified recurrence in the current build73 implementation. A read-only audit of
the connected construction, publication and persistence path found:

- `RemapAtlasSurface` copies an unchanged perimeter contract; new internal smooth
  paths depend on fixed original geometry and current endpoint associations.
- `RebuildTraceCoverage` receives a **const** trace map and produces face covers
  only. Its generated microtriangle boundary knots do not replace prescribed
  trace records. `RebuildReferenceTriangulation` starts again from original
  facets, current associations, prescribed paths and the simplified region border.
- `BoundaryParameterState::Prepare/Commit` stores supplied traces, rather than
  deriving new persistent paths from reconstructed face interiors. Schema8 reload
  preserves those records and reconstructs derived indices.
- Entry `RebuildWorkingSurface` replaces noncanonical or unnecessarily subdivided
  legacy paths together with both incident faces and current original-support
  footprint. All affected volume owners advance versions through Directory;
  donors are frozen before this geometry normalization.

The existing 16-event actual neighboring-move/publication/source-envelope/reload
case passes in serial and six ranks, including build73 MPI (69.929s, unchanged
pins). Its per-phase descriptor/arithmetic/storage checks are scoped evidence,
not a general complexity theorem. Endpoint precision, current original-region
size, genuine feature intervals and actual current intersections still govern
cost. The next regression combines legacy shared-edge normalization with that
same repeated operator/publication/reload sequence; it is not a new routing rule.

A distinct limitation remains: independently canonicalized paths are proposals,
not joint graph recovery. If an internal proposal crosses a retained perimeter
whose endpoints and canonical paths are unchanged, adding neighboring metadata
alone cannot admit it. A geometry-delta sidecar would require either a demonstrated
noncanonical interface or a joint network rule that actually changes the network.
It is **not being added as a remedy for an unproven remaining recurrence**.

The current difficult-target failures must remain separate: source73 curved CFD
runtime is rejected after 17 splits and no collapses; source72 M6 is terminal and
unaccepted. The captured curved runtime state has no explicit covers/traces, so
that capture does not demonstrate retained-descriptor growth. Many trials fail
the geometry-error gate and repeated unsuccessful construction consumes work.
No accepted curved CFD mesh, conservative workflow or M6 restart is established.
The receipt audit further narrows that failure: every split pass through split17
made progress. Work rose from56,519,137 after split17 to64,680,086 after the
following zero-success collapse pass, then reached67,207,168 during the next split
pass. Existing facet-interior points disprove an original-edge-only sampling
limitation. Final fallback reasons near exhaustion cannot establish geometric
infeasibility. A fresh-work replay of the remaining demanded edge16–6496, retaining
the first operator reason, is the decisive next check before changing construction
or scheduling. Approximate positive-child/error calculations are not an operator
certificate. The next design must address demonstrated behavior without raising
limits or treating arithmetic optimization as architecture repair.

## Historical recurrence and its replacement

Build36 serial campaign `staged57/cases/serial_full_20261027T153436_2673844`
finished in99.7804s with unchanged pins:226/228 cases passed,1129639/1129642
assertions. The overlapping-star test exhausted arithmetic on event5. At that
point clipping-derived paths were imported by later neighboring transactions;
discarding interior cuts alone did not remove their perimeter recurrence.
Build37–40 introduced fresh fixed-reference paths and prescribed-network
reconstruction; build62–64 added coordinated legacy normalization and owner
publication. Historical sections below retain the design and failed experiments,
not current status. DC-17/DC-18 remain open.

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

## Build47: ownership and scheduling integration (validation pending)

The build46 small workflow's zero explicit covers/traces distinguishes its
failure from retained-path growth. Its queue nevertheless performs complete
projection/coverage checks for all hypothetical split children before selection.
Build47 removes that custom eager feasibility screen: each current bad physical
face contributes geometry-excess squared times physical edge-span squared to
its edge demands. This priority is a custom heuristic, not a convergence proof.
The established scheduling separation is to enumerate demand, reserve/select a
complete star, then prove the chosen replacement. GeometryRefine retains its
strict before/after geometry progress and all validity/publication gates. A
serviced edge retires from the generation; later frontier reseeding owns retry.

Accepted-face proof ownership is shared by containment, parameter export and
PrepareCurvedAtlas. Exact oriented topology, all point/binding coordinate bits,
current state revision and absence of applicable overrides establish reuse.
Publication keeps accepted before coverage in the region chain without repeating
its per-face proof; every after face remains fully validated. Load validation
and frozen donor restricted envelopes remain complete. No serialized flag,
cache, epoch or archive format is added.

First mobility activation previously copied legacy state whose chart containment
was weaker than the actual-chord positive-projection certificate. Build47 proves
non-original subfaces once at activation, including policy and retained traces.
Exact cyclic original-facet faces inherit the authority constructor's certificate
only with valid dimension-zero binding features and exact reference coordinates,
and without applicable overrides. Pending-query provenance now follows actual
pending records, so real UINT64_MAX mesh IDs cannot hide malformed edge contracts.

This is one connected current-state ownership/scheduling change, not another
path-routing fallback or an arithmetic limit increase. Its complete-workflow,
repeated-edit, source/publication/archive and M6 validation is still pending.
See [verification cost and trust boundaries](GEOMETRY_VERIFICATION_COST.md).

## Connected closure review after build53

The current implementation no longer feeds newly clipped interior subdivisions
back into shared surface paths: interior/replaced paths use the fixed original
charts and current endpoint associations; RebuildTraceCoverage takes its paths
as const input. This removes the diagnosed recurrence in that scope. It does
not prove arbitrary graph routing or normalize all old boundary contracts.

An inherited local perimeter path is still copied unchanged. Publication checks
that a changed shared path includes every accepted incident face and every owning
volume row, but rejection is not closure assembly. A legacy noncanonical path can
therefore keep constraining subsequent patches. Genuine closed feature curves
also need an oriented model-arc association: endpoints alone cannot select one
of two arcs. Preserve that topological information rather than calling it history.

The next connected regression uses actual neighboring volume moves, Directory
publication and schema8 decode continuation. The earlier16-event overlap fixture
only remaps a private surface and constructs new parameter states directly; its
stable counts/work do not qualify the distributed pipeline. Full CFD transfer,
solution restart and accepted M6 remain separate required checks.

A candidate normalization design is a geometry-only transaction before local
construction: identify obsolete nonfeature contracts against the fixed reference,
collect both incident faces and versioned volume owners, reconstruct the shared
network without moving the physical chord mesh, then publish all changed covers,
paths and owner versions atomically. This reuses existing metadata publication
and leaves strict volume-operator input contracts intact. The closure must cover
the new path's original-geometry footprint, not merely its two old incident faces;
otherwise a route may leave that union. Plan that closure from current associations
and the original geometry before reconstruction, rather than trying larger local
patches after failures. Any support index/workspace must be admitted and discarded
or rebuilt from current state. No budget increase or routing fallback is proposed.
This is a design under review, not an implemented capability.

The direct reusable reconstruction is RebuildTraceCoverage with the already
planned immutable path map. PrepareCurvedAtlas with moving=nullopt does not run
the moving-sheet disk tests: its blanket header wording about holes is stale.
The existing reference triangulator handles union holes/components; each CFD
face still must be a connected disk. Avoid RemapAtlasSurface for a planned
normalization because it would choose the paths again and reparameterize curves.

A draft global normalizer is not merged: scanning/replanning the entire current
boundary at each event removes history dependence but does not establish local
cost. The connected local design needs an admitted current original-support to
working-face index, maintained/rebuilt with accepted associations and publication.
No prototype of that scan is accepted as the solution to the locality obligation.

Draft review also found that canonical-planning every clean implicit edge can
refuse an already valid mesh solely because the custom route proposal fails.
Byte identity would request changes even for semantically equal traces. Existing
edge_incident stores one representative face, not a complete two-face closure;
reuse Directory edge-star queries or extend the current support/incidence index.
The draft's undocumented aggregate topology allocation and worst-integer per-piece
charges also need replacement with actual admitted storage bounds. These are
reasons not to merge it. Preserve the established original geometry/current
association design while resolving closure and routing as a connected capability.

## Local closure design prepared during the build61 M6 run

The revised draft streams current explicit contracts; it does not canonicalize
clean implicit edges or allocate an entire temporary boundary graph. Two derived
indices provide both incident faces and the current faces covering an original
facet. Constructor/load/rebind rebuild the indices; publication updates only their
changed records with allocation-free node transfer. Component queries use the
same canonical entity spelling throughout.

A candidate path uses the immutable original geometry and current endpoint
associations. The planner gathers both incident faces and the current faces in
its original support/vertex-star footprint before reconstruction. Genuine curve
arcs remain fixed. Equal existing paths need replacement only when their stored
complexity exceeds the current fixed-reference proposal. A typed trace comparison
separates valid inequality from malformed input or resource refusal.

The existing RebuildTraceCoverage receives the complete, already planned path
network. Its positivity, per-face disk, shared-boundary and old/new region proofs
remain mandatory. Independent canonical paths remain proposals: a crossing or
protected-feature conflict refuses the transaction; no alternate-route retry is
added. The original-support index must cover seam and vertex contacts as well as
facet interiors.

The engine freezes donors before any private normalization. It republishes all
affected unchanged physical cells with advanced versions and their coordinated
geometry records before current-target selection. Source reconstruction uses the
frozen donor interpretation; transferred/restarted target state uses the accepted
working association generation. This avoids broadening strict volume-star operator
inputs merely to carry geometry-only neighbors.

The coordinated closure, typed trace comparison, admitted descriptor copies and derived indices passed the baseline build62 normalization case. Expanded source63 normalization variants pass serial and MPI6; build61 M6 remains terminal and unaccepted.
Acceptance still requires a legacy noncanonical shared edge refreshed with its
incident owners, actual neighboring operations/publication/source/reload over
repeated events, accepted M6 and complete solution transfer/restart. The passing
16-event MPI workflow qualifies the present canonical path flow, not this new
legacy-closure implementation or full CFD/M6 completion.


## Build61 difficult-target diagnosis and next staging decision

The full M6 run rejected at its unchanged elapsed-time limit: 303,359 working
cells became 1,376,207 private cells, with 877,469 remaining metric demands.
The 146,862 source-envelope records are a separate count. About 99.1% of
attempted construction proposals succeeded; geometry demand was zero at exit,
and there was no quality-stage entry, relocation, reconnection or coarsening.
These counters establish a one-direction schedule and unresolved cost/growth,
not an inability to create valid cells or proof of excess final resolution.
The target complexity is 500,000, not a tetrahedron-count ceiling.

Omega_h's length-satisfaction loop alternates refinement and coarsening before
quality repair (the linked official adaptation source, lines160–175). Therefore
absence of quality entry alone is not the fault. Native currently finishes
refinement and its quality closure before enabling coarsening. The next staging
design must consider bidirectional metric-length satisfaction on the current
mesh, retaining validity-only construction and a later metric/BL quality stage.
Do not replace this diagnosis with an early quality veto, an arbitrary cell cap,
a larger timeout or another arithmetic-only optimization. Geometry normalization
cannot resolve this first-event scheduling limitation by itself.

The [metric-length design and measured contact cost](METRIC_LENGTH_SATISFACTION_DESIGN.md)
records the proposed shared-lifetime schedule, affected callers, safety hazards and
acceptance conditions. It is not yet a validated implementation.

## Current normalization qualification

Build63 PASS66.1366s; all 17 build PIDs gone. Nineteen compiled filter audits
pass (245 serial/222 MPI selected). Normalization passes serial 745 assertions
in 15.9078s and MPI6 in 16.796s (727/721/721/721/715/715 assertions); all nine
MPI PIDs gone. Repeated 16-event MPI publication/source-bound/schema8 test passes
75.9713s; all nine PIDs gone. Its cost receipt records 12–20 face pieces, 13–19
traces, 56–109 bits, 30,540,601–67,213,767 work excluding decode and 10,058,008–
46,571,848 bytes aggregate admitted workspace. Sampled process-tree RSS ~214 MB
is not peak allocation. Source-envelope checks are not CFD solution transfer.

Broad serial `staged57/cases/serial_full_20261027T230545_3953311` FAIL176.0948s:
243/245 cases and 1,134,704/1,134,707 assertions pass. Two failures are the known
curved runtime; the third is a stale quality-stage query expectation (6 vs 12).
Broad MPI63 `staged57/cases/mpi6_20261027T231001_3964969` FAIL177.1677s
with unchanged pins: each rank passed 220/222 cases, with the same three failed
assertions; the empty-rank fixture passed and all nine processes exited. Source64
corrects the stale fixture expectation and strengthens row/version/content checks.
Build64 PASS96.1853s; focused quality_cache serial and MPI6 pass in 6.8561s and
6.8769s respectively, all owned processes gone. All 21 compiled filter selections pass in build64; no owned job remains live. These focused passes do not qualify the failed
broad campaigns. No complete M6 or CFD transfer/restart is accepted.

## Geometry sizing and source73 scope

Current `CBoundaryLayerMetric.cpp:1989–2007` applies a Hausdorff-derived
tangent metric floor only when the native wall has analytic authority: sphere
and cylinder (disk excluded). `FromNativeReference:2095–2126` attaches that
authority and tolerance only for a complete analytic wall component. The cubic
backend exposes local curvature, but this wall metric path does not consume it;
`CNativeMetricFamily3D.hpp` stores selected/frozen metric and optional layers.
`CNativeRemesher3D.cpp:192` passes ADAP_HAUSD as engine surface tolerance for
geometry checks. These inspected paths do not establish a generic cubic
Hausdorff-derived Euler or BL metric floor before coarsening.

[MMG anisotropic sizing](https://raw.githubusercontent.com/MmgTools/mmg/master/src/mmg3d/anisosiz_3d.c)
contains geometry/Hausdorff sizing, while its
[collapse implementation](https://raw.githubusercontent.com/MmgTools/mmg/master/src/mmg3d/colver_3d.c)
checks boundary edges before committing. [Geometry Central's intrinsic interface](https://github.com/nmwsharp/geometry-central/blob/master/include/geometrycentral/surface/intrinsic_triangulation.h)
separates input and intrinsic triangulations and exposes removal of inserted vertices;
that API alone is not a drop-in for moving/removing original CFD vertices. These
are design comparisons, not proof that native construction is qualified.

Source73 MPI overlap publication `staged57/cases/overlap_publication6_20261028T025843_383399`
PASS69.929s with unchanged pins; mapped trace
`staged57/cases/mapped_work_trace_20261028T024728_354109` FAIL12.827s with
unchanged pins. Full accepted curved workflow and M6 remain open; no new M6 run
is planned until construction's failure is understood.

The unbuilt source74 endpoint-curvature sizing proposal was removed after review: it was not a necessary Hausdorff veto. The next design is shared-edge closure, not an additional local policy. Source73 SAT and coverage changes remain; full workflow qualification is open.
