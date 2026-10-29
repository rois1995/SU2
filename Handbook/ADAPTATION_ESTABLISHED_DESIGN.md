# Established adaptation foundation — 2026-10-29

## Requirements versus decisions

User requirements are native distributed SU2 integration, validity-first construction
then metric/BL improvement, movable boundaries on fixed geometry, genuine features
and separate bodies, geometric tolerance independent of h0, conservative transfer,
history/restart and practical repeated-event cost. None prescribes rational covers,
smooth paths, source prisms, certificates, schedules or numerical budget formulas.
Current numeric acceptance settings remain comparison baselines, not newly attributed
user requirements. DC-17/DC-18 and the complete curved/M6 workflow remain open.

## Complete surface bounds and two-event M6 — 2026-10-29

Immediate user target: an accepted M6 Euler mesh and CFD → mesh → CFD → mesh.
Every checkpoint is authorized for pushing. The previous local commits are now
pushed; the former automatic push denial is historical, not a current blocker.

Build129 adds bounded tracing to the existing surface-union query. Its diagnostic
case mapped_work_trace_20261029T143141_1355812 identifies the expensive direction:
the complete two-sided geometry check is subdividing the original model against
the working surface (model pieces1024). At more than17,000 queued regions, the
lower bound is~.000715 below tolerance.001, while the conservative upper bound
remains above it. The owned diagnostic is stopped after collecting this evidence
(exit143,267.747s,pins unchanged); this is not a completed acceptance run. The
build126 outer-timeout failure remains separate evidence. Last stage censuses do
not identify the state inside the final geometry check.

The existing upper witness covers a control hull by one target triangle or an
exactly coplanar connected union. Slightly noncoplanar target triangles therefore
lose the union witness at seams. Reuse the exact finite projected footprint for
connected triangles sharing a dominant projection axis and orientation, and bound
all overlapping target-face lifts with outward interval arithmetic. A covered
projected rectangle supplies a target point; the maximum vertical-plane distance,
target approximation error and clamping displacement bound its distance. Holes and
failed projected footprints cannot supply this witness; the existing individual
triangle bound remains. This changes the enclosure, not the prescribed surface,
tolerance, topology or final two-sided acceptance. Footprint predicates are now
metered as well (after each finite footprint query; it is not a wall-clock deadline).

Scope: TriangleUnion and its finite-plane containment overloads, existing geometry
backend regressions, and the pinned M6 harness. New curved/sheared graph and missing
coverage regressions accompany the existing wrong-body/hole/curvature checks.
Build130 completed with immutable pins (1546.670s). Its existing curved query
exposed an oversized optional projected-reference reservation, and the new fixture
used interior bindings at edge/vertex points. Build131 fixes those without raising
limits; build132 reuses the existing triangle tree for relevant projected faces.
Build132 passes (63.160s,1295 pins unchanged). Geometry checks pass15/16 cases;
the new sheared graph's complete/missing-coverage sections exhaust their work
reservations (1,048,576/4,194,304), preserved under
cases/geometry_backend_20261029T151455_1472920 and
cases/geometry_graph_trace_20261029T152116_1488543. These remain unresolved
diagnostics, not justification for another checker cycle before M6.

The existing M6 driver loop supports two events with ADAP_SUBITER=(2), preserving
the same metric target and per-event gates/budgets. Its harness now optionally
checks both six-rank native summaries, ordered conservative transfer/replacement,
both meshes and geometry sidecars, and actual next-cycle solve history. Completed
events save independent output hashes and provenance; whole-run success requires
all planned events. Native summaries alone cannot establish the CFD workflow.
The helper self-checks pass, including historical M6 rejection. Actual restart and
unsteady history qualification remain separate obligations.

### Full M6 result and collapse admission — build133

Build132's first event failed at~1356s: `Surface-union parameters exhausted exact
 dyadic resolution; bounds unresolved`. It made158 private accepted collapses at
 global work167 but completed no length pass or quality stage. No mesh publication
or conservative transfer occurred. Selection consumed731s, including682s nested
collapse geometry on rank4; every owned/cache count matched at failure. These
measurements implicate the single-cell selection shortcut; no throwing stack or
exact edge was captured. The remainder was stopped after this known failure,
all owned handles are terminal and pins unchanged. Detailed evidence is in
staged57/build132_m6_summary.json.

CollapseConstraintViolation previously ran general surface-union enclosure for a
hypothetical single-cell remap. Defer that deviation query when current bindings
do not support its original facet. Keep direct facet deviation, ContainsFace and
height checks. Complete CollapseVertex still checks the complete classified star,
exact positivity, interface/features and surface; private admission and final
two-sided certificates remain mandatory. This is a selection shortcut, not mesh
admission. A later cavity query may still reject/stall; M6 must establish viability.
Affected callers are shared Selection::FilterCollapse and existing filter tests.
No limits, tolerance, final gates or geometry authority changed.

Build133 passes93.226s with1295 pins unchanged. The existing remapped-face fixture
now tests deferral with no remaining atlas budget, retaining its measured full
cavity error. It passes. Filter selection passes4/5 cases98/99 assertions: the
unchanged analytic-facet path expects a specific older rejection substring but
receives no-admissible quality/length change; rejection/cache/input checks pass.
The test's stale message assumption is corrected in build134 to the actual
round-budget stop/reason, retaining rejected-result/cache/input checks.

### Full M6 timeout and shared exact-predicate pruning — build134

Full build133 M6 rejected at1801.84s with metric-update elapsed-time exhaustion,
319 private accepted collapses/work377 and no completed length pass or quality
stage. Selection1.1s/nested filter<.7s confirms the previous filter cost is removed.
Update465s and other1117s dominate; current cache is partial at the timeout.
Five-second low-frequency samples show `Selection::Add → ParameterError →
BoundPhysicalSurface → QuerySurfaceUnion` and `QuerySurfaceUnion → TriangleUnion
→ FinitePlaneReference → PlanarReference → OnEdge`. These are sampled paths, not
whole-event cost attribution; DWARF decoding is partial. Fixed target approximation
could leave a bound uncertainty floor, but this run does not prove that hypothesis.
No accepted mesh/transfer; remaining event on original mesh stopped after failure,
all owned handles terminal, source/runtime pins unchanged. Receipt:
staged57/build133_m6_summary.json.

The next connected change prunes coordinate-separated queries before the same
exact predicates in shared planar construction/containment: point-on-edge boxes,
nonconvex boundary-pair boxes, boundary/chord boxes and boundary/triangle boxes.
This addresses the sampled constructor as well as repeated distance-witness
containment, without replacing either algorithm or weakening contact checks.
Strict coordinate inequalities retain all touching/overlapping boxes. Existing
hole/concavity/orientation tests and one-ULP hole-boundary segments check the risk.
Build134 passes162.397s with1295 pins unchanged. Focused planar-contact/filter
checks pass8 cases/190 assertions, including one-ULP hole-boundary segments.
Full M6/build134 rejects at its metric-update elapsed-time limit with414 private
collapses/work503, no completed pass/quality stage/mesh/transfer. Selection1.66s,
update474s,other1129s. Source/runtime pins unchanged; remaining retained-mesh
event stopped and handles terminal. The pruning helped but does not establish
viability. Perf and read-only GDB snapshots identify current-cell surface queries,
projected reference setup and model approximation in complete-cavity guidance.
GDB catches nextafterl at negative long-double denorm_min; query inputs/bounds are
optimized out. Its brief pause perturbs timing and the fixed-target uncertainty
floor remains unproved. Receipt: staged57/build134_m6_summary.json.

### Reuse immutable physical certificates and exact interval identities — build135

BoundPhysicalSurface was recomputing identical geometry across cavity guidance and
current-cell census. Reuse its successful conservative double bound on the same
immutable authority, keyed by the entire ordered facet list, complete physical
triangles and tolerance bits. A separate map shares the existing FaceMemo byte
pool with direct-facet queries; bounded eviction clears both maps. Variable key,
node/bucket and temporary storage are included. Cache misses use the same query;
exceptions are not cached. Final two-sided QuerySurfaceUnion is unchanged and
mandatory. No new API, scenario dispatch, tolerance or work/time limit.

Exact zero/unit arithmetic identities avoid manufacturing denormal intervals in
zero-heavy cubic evaluations. Scalar zero multiplication requires finite operands;
non-identity sums/products retain nextafter enclosure, including genuine underflow.
Singleton range products use the existing scalar path. Equal finite binary64
subtraction is exactly zero. New tests check query-key isolation across coordinates,
support/body and tolerance, reused work and storage admission. Existing two-sided
hole/wrong-body/curvature tests exercise publication obligations. A source-extracted
probe checks648 enclosures against exact Boost rationals, underflow and nonfinite
cases; compile flag was corrected from C++11 to repo C++17 without source changes.

Build135 passes84.211s/1295 pins unchanged. Focused24/25 cases703/704 assertions;
only prior sheared-graph complete-section work exhaustion remains, not a claimed
full-suite pass. Full M6/build135 rejects metric-update elapsed time with760 private accepted
collapses/work965 and no completed length pass/quality stage/mesh/transfer. Update
394s and other1330s dominate; selection<1s. The perf sample still shows complete-
cavity surface queries and projected-reference setup. Remaining retained-mesh event
stopped; all owned handles terminal, pins unchanged. Receipt:
staged57/build135_m6_summary.json.

### Budget contract and coherent generous M6 freeze — build136

Review identifies an API budget gap: memo hits returned before checking sticky
atlas denial or charging key construction/lookup. The engine collective budget
vote and final certificates still prohibit mesh acceptance after exhaustion. Charge
all admitted key words before allocation/lookup; cache hits consume only that work,
while misses use the original query. Test empty, sticky and already-overrun budgets
as well as full key isolation/storage checks. No certificate or geometry change.

Build136 passes84.212s/1295 pins unchanged. Focused6 cases/123 assertions pass.
The interval arithmetic block is unchanged; reuse135's648 exact-rational enclosure
checks with matched extracted hash. Prior sheared-graph complete-section work
exhaustion remains recorded. No new numerical acceptance gate, tolerance or target
metric field. One-event diagnostic config stays1800s/2400s.

The previous1800s attempts admitted158→319→414→760 private collapses without
completing a pass. Build136 was frozen with5400s native/event,14400s outer; after
3202.123s the user explicitly requested stopping and fixing repeated preparation.
The owned attempt is terminal, pins unchanged, all handles gone. No native result
or geometric rejection inferred from the stop. Receipt: staged57/build136_m6_summary.json.

### Retained fixed-surface query hierarchy — builds137/138

The authority already stores exact cubic control nets and a control-hull ADT;
QuerySurfaceUnion rebuilt the tessellation, projected footprints and triangle ADTs
for every changing candidate. Cache of identical answers did not address this.
Retain certified per-original-facet approximations/footprints/ADTs by facet and
requested tolerance under CurvedSearch. The existing root index selects leaves;
exact requested-facet filtering preserves body/marker/sheet scope. A query view
combines conservative per-facet lower/upper distance bounds, so no target outside
that scope can certify a candidate. Working-surface target indices still rebuild
when the working mesh changes. No new archive schema or solver/scenario dispatch.

TriangleUnion now receives current AtlasOps in its query methods instead of
retaining a reference to a construction-time stack object. A preparation mutex
protects retained traversal scratch; the existing root ADT mutex protects shared
root traversal. A pre-admitted pool, counted in ResidentBytes, retains fixed data;
if an entry cannot be retained, its current-query storage is explicitly admitted.
Full geometry gates stay unchanged; no failed certificate is promoted to success.
Per-facet upper bounds can be looser at original seams than a combined footprint;
existing seam/hole tests and M6 determine viability, never a relaxed tolerance.

Build137 passes306.746s/1295 pins unchanged. New different-triangle and overlapping
support reuse checks pass2 cases/32 assertions, comparing against fresh authorities.
The broader51-case campaign passes49 cases/17559 assertions;14 assertions identify
reservation bounds growing2 bytes when decoded revision text gains a digit. The
reservation had depended on decoder ceiling, which includes archive text length.
Use a fixed curve-count bound when it can be admitted, preserving strict repeated-
publication assertions. One prior sheared-graph work-exhaustion failure remains.
Build138 passes but its undersized pool failed reuse. Build139 separately admits
a geometry-sized cache quota at load, passes1018.648s/pins unchanged (3 workers),
and passes2 reuse cases/34 assertions with retained curved indices. Connected
geometry/publication and MPI remain pending. Tests also add a tighter-tolerance fresh-
authority comparison. After resource/geometry and bounded MPI validation, retry
full M6 with5400s native/event and14400s outer. No accepted M6 workflow yet.

## Residual length repair after shape closure — 2026-10-29 continuation

The stopped mapped workflow repeatedly closes shape debt, returns to construction,
and recreates it while retaining size debt. Bidirectional improvement queued only
shape/height users; its existing optimizer's length-removal/insertion operations
could not see size-only cavities. Add a globally synchronized residual length pass
after the shape/height census closes. Queue only oversized-edge features, import
their complete distributed cavities, and use the existing optimizer and atomic
publication. Preserve geometry, final shape and changed-edge excess admission.
Reenter construction after exhausted repair demands; no work limit or final gate
changes. The separate public Repair and standalone coarsening semantics stay.

Build125 establishes the ordering in serial (5 cases/883 assertions), but its
initial all-feature size queue is rejected by the difficult mapped workflow:
geometry arithmetic exhausts at 67,207,168 with only876 proposals,24 private cells,
17 size failures and11 geometry failures. That strategy is not retained. Build126
restricts size demand to oversized edges and skips the shape-only primitive trials;
it also admits queue storage before allocation. Its difficult Euler run reaches208
private cells with no shape/geometry failures and3 size failures at2468 proposals,
then remains inside construction geometry work beyond the120s event limit. This
is private progress, not acceptance. Preserve
the first failure under staged57/cases/mapped_work_trace_20261029T140408_1285674.

The new run is staged57/cases/mapped_work_trace_20261029T140728_1294084. Two
completed QuerySurfaceUnion calls consume5,615,551 and1,974,496 arithmetic work
units. The active query is not identified by those completed-call receipts; a
debugger attachment was denied by host ptrace policy. QuerySurfaceUnion performs
model tessellation and directed subdivision until upper/lower bounds decide the
tolerance. Event time is checked outside that synchronous operation. In addition,
TriangleUnion charges a plane visit but its finite-plane footprint containment
traverses components/boundaries without the atlas budget. That is an accounting
gap, not proof of the observed bottleneck. Capture query direction, subdivisions,
bounds and footprint work before changing this mechanism; do not raise budgets
or infer geometric impossibility from the observation timeout.

That run terminates at the unchanged600s outer timeout (605.476s harness receipt,
exit124); source/runtime pins remain unchanged and all owned handles are terminal.
Build127 adds ordinary Repair admission during residual size work even with wall
constraints: already-satisfied height improvement cannot bypass its size/shape
merit. Serial45/9611 passes. MPI6 accepts all tested meshes and passes the new
case, but the existing first-rejection assertion fails: optimizer-only refusals
were counted without invoking the capture hook. Build128 supplies an explicit
optimizer refusal reason and routes residual-size refusals through that hook;
the assertion is retained. Build128 passes (94.897s,1295 unchanged pins), connected
serial45 cases/9611 assertions passes (9.667s), and focused MPI6 passes five cases
per rank (227–631 assertions/rank,5.584s). All owned handles are terminal. These
checks include the residual size stage and first-refusal capture, not full curved
acceptance. The difficult workflow is not rerun after the height/diagnostic-only
corrections; build126 failure provenance remains separate. Compact receipt:
staged57/checkpoint128_euler_size_summary.json under
integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/.

Scope is the shared engine selection/stage dispatch, its distributed regressions
and existing pinned harness. No new config, geometry backend, persistence format
or solver-specific reconstruction is needed. Acceptance remains a complete curved
Euler mesh, conservative transfer/history and actual solver continuation, then
early M6 qualification; this stage change alone cannot close DC-17/DC-18.

## Selected foundation and actual source inspection

Use SCOREC/MeshAdapt's classified-cavity and nodal size-field design **inside SU2**.
Retain SU2's operators, owner/version Directory, cubic geometry backend and separate
conservative CFD projection. Do not invoke an external remeshing executable.

Reference checkouts (reference-only; no new dependency):

- SCOREC/core fd4386fddb3543b7dbc9cfe41f98af04a57a3df7: ma/ma.cc 23–44
  coarsen/refine/snap then shape improvement; ma/maSize.cc 365–540 transports
  nodal anisotropic/log metrics; ma/maSolutionTransferHelper.cc 35–120 chooses
  the old cavity parent by maximum barycentric insideness and interpolates fields.
  apf/apfCavityOp.cc 96–136 obtains cavity locality and resolves ownership;
  ma/maCollapse.cc 27–39 requests endpoint cavities. Classification constrains
  geometry. Root LICENSE is BSD-3-Clause; ma/LICENSE and ma.cc specify a non-commercial
  license. No MeshAdapt source is copied into SU2.
- MMG 8ed2259164fa4c90be6301d247ecb1db7bd61228: libmmg3d.c 975;
  mmg3d1.c split/collapse drivers; movpt_3d.c 483/1263 boundary movement;
  delaunay_3d.c 581 anisotropic cavity; bezier_3d.c 326 derives geometry from
  current-mesh normals. No fixed-original projection callback found in public API.
- ParMmg d2eddc502cae46084ca2947859f333d7a7b23943: libparmmg.c 192
  calls local MMG; tag_pmmg.c 39–187 protects/releases artificial interfaces;
  libparmmg1.c repartitions and rebuilds communicators. MMG/ParMmg LGPL-3+.

Paths live under integration_evidence/native_3d_bl_v1/established_adapter_sources_20261028.
Unmodified SCOREC also measures/vetoes collapse quality during construction; that
policy is deliberately not adopted. Its default field transfer is not evidence
of conservative CFD transfer. Its evaluable-model parameter interface would require
a nontrivial adapter for SU2 facet/u/v cubic associations. Implementing its design
in existing SU2 facilities avoids that adapter and preserves native integration.

## Explicit stage admission independent of builder mode, build114

Build113 mixed-debt regression passes, but actual curved workflow still hits4096
proposals (atlas work27.39m, not its67.21m limit). Last improvement q/J bad cells
rise1/2→7/5. Geometry builders intentionally stay ValidityOnly during the event,
including when constructing quality proposals; changing that global mode would
reintroduce premature generation floors. The helper's Absolute repair policy is
therefore not production stage admission.

Apply the staged improvement decision at the existing shared metric_trial boundary
for local and streamed ordinary Repair proposals. Use already measured diagnosis:
existing worst normalized q/J improvement or passing final shape, and nonincreasing
changed-edge metric excess. Real requested-height progress retains its existing
explicit GeometryRepair route; do not dispatch on solver/fixture identity. No new
shape/geometry measurements in construction. The serial captured workflow observes
its actual clipped worst shape score through each no-height quality phase and
asserts non-regression, while still requiring complete final mesh and transfer.
This merit rule is an implementation choice responding to captured churn, not a
new user-imposed final threshold. Full BL/RANS qualification remains required.

## Captured improvement/constraint coupling, build113

Mapped112 phase receipts show bounded descriptors in this event (<=19 associations,
34 working faces,21 trace pieces,2 ratio limbs). Last improvement consumes~85% of
67,207,168 work while q debt rises .07617→1.24217 and J debt .32706→1.51453.
This is not evidence of retained representation growth in this run. Selection
queues combined invalidity, so size/geometry-only cells compete with shape debt;
GeometryRepair promotion can admit worse shape for geometric progress.

Use existing quality_valid (measured q/J and requested height) for production
AdaptMetric improvement demands, synchronized at phase entry, published insertion
and quality refresh even when combined validity does not change. Keep combined
validity and all final counters strict. Public standalone Repair retains its full
repair-demand semantics. During staged improvement, ordinary Repair admission
requires shape progress or shape-gate closure when shape is unacceptable; size
progress alone may not degrade shape. Use existing measured values, preserve
metric excess bounds and height checks, and keep construction validity-only.
Do not promote staged shape repair into geometry-only progress. No new representation,
case/physics dispatch, work cap or fallback. A mixed-body serial/MPI test guards
quality demand on one rank while another has only size debt. Full smaller/M6 mesh,
transfer and restart remain the acceptance conditions, not this test.

## Captured stage starvation and established iteration staging

Build106 M6 performs two length-pass pairs and11567 private edits, yet reaches no
quality stage within1800s. Small build107 makes106 length-pass pairs before its
only quality stage, then exhausts work. The driver requiring an unproductive pair
before improvement is an agent-selected rule, not a user requirement or a condition
of classified cavities. ma/ma.cc adaptVerbose lines75–94 instead calls
fixElementShapes after each coarsen/refine/snap iteration. Its ordinary adapt
routine has a bounded maximumIterations loop before shape improvement; neither
requires unproductive length passes. Use the existing SU2 improvement stage after
each complete collapse/split pair, then its existing return to construction where
needed. Remove the redundant pair-productivity flag. Construction remains free of
q/J measurements and ranking; final acceptance and every transfer/restart gate stay.
The existing distributed opposing-length workflow must enter improvement after
one pair and still finish an accepted mesh. Curved/M6 qualification remains open.

## 2026-10-29: close the captured projection and association paths

The exact M6 failure is numerical: seven permitted wing-sheet patches fail in
world coordinates; the identical existing minimizer converges on six after local
translation and uniform scaling, including the current surface at distance4.78e-11.
Keep the original nets authoritative and realize the returned parameters there.
This is conditioning of the selected projector, not another projection fallback.

Ordinary associations use validated version/entity/parameters. Avoid reconstructing
rational endpoint keys to compare those same values, and search feature routes only
when both endpoints are classified on genuine lower-dimensional entities. Keep
feature continuity, body/sheet incidence, owner publication and restart checks.
Unavailable proposals may refuse; invalid state and resource errors remain fatal
at existing collective votes. Unknown geometry cannot count as a measured candidate.

Acceptance remains the complete smaller curved and M6 workflows: actual movement,
neighboring edits, accepted mesh, conservative current/history transfer, repeated
events and solver restart. Primitive tests support this assessment only. No limits,
quality gates or scheduling rules change for this failure. Cost measurements must
show whether the removed ordinary key work materially reduces repeated-event cost.

## Demonstrated limitation and connected implementation

M6 build92 rejects before operators: folded/degenerate joint original-source prism,
face734, 386.112s total. Those prisms contain **sensor metric tensors only**;
they are not the conservative CFD transfer mesh. Requiring a globally conforming
prismatic extension just to measure the metric is an unnecessary coupling.
Captured small curved failure also builds valid cells with fresh work; neither
conservative geometric rejection nor exhaustion establishes impossibility.

Replace metric-support prisms with current-mesh nodal metric transport through
complete old cavities. Preserve surviving node values; interpolate new nodes from
the best old parent. Convex clamped P1 interpolation preserves SPD outside the old
chordal cavity after constrained projection; this interpolation policy is an SU2
implementation decision, not an exact copy of SCOREC log-size interpolation.
Measure replacement cells against their transported nodal field. Keep geometry
validity and final geometry checks separate from metric sampling.

Affected chain: ImportGeometry seeds attributes; OwnedCell/Directory/partition
carry them atomically; engine selection/private measurement/publication transports
them; WorkingWeights, reassessment, normalization and ReaderSlices consume the same
field. CFD conservative projection and history/restart keep independent acceptance.
The .native_ref geometry format need not change for transient metric attributes;
the internal OwnedCell wire record gets an explicit version to reject stale payloads.

**Identified dependency:** existing candidate normalization binds every quadrature
point to original donor IDs and recomputes nonlinear original-family trials. Nodal
transport cannot simply substitute candidate IDs there or leave stale trial stencils.
The connected implementation replaces that agent-selected original-family policy
with bounded uniform scaling of the transported current nodal field. Global
spectral bounds preserve hmin/hmax; uniform scaling preserves tensor anisotropy.
The same prepared samples and BL geometry composition assess trials, and all
working node attributes update together before the second adaptation stage.
Current source stencils are temporary per-stage quadrature data, never ancestry.
No new source-hull fallback or arithmetic-cap increase is planned. Build94 is
pending: this is an implementation decision awaiting integration validation.

## Acceptance

First: small curved Euler and M6 Euler accepted adaptation, final geometry/q/J/size,
conservative state/history transfer and restart under comparable settings. M6 early.
Then requested BL/RANS through general metric/height operators. Also require actual
boundary movement, overlapping neighboring edits, repeated events, MPI publication,
correct selected tests and runtime/memory/current descriptor/work counts across events.
Primitive checks support integration; they do not close this milestone.

## Current implementation checkpoint

Current-nodal field edges use endpoint-linear metric interpolation and existing
analytic/composed integrators; they do not trace global old-donor coverage. Their
point evaluator preserves exact nodal values and uses bounded best-parent convex
interpolation elsewhere. Only metric interpretation changes; no geometry validity
permission follows from this evaluator. Legacy original-field queries remain
strict for separate callers. The region constructor still has a strict stage
snapshot path; BL regional mobility and FIRST_ROW stage-two sensor construction
remain unqualified and must be connected before BL completion.

Normalization core units were verified in CSolver.cpp and NativeRemesher3D.cpp:
core is an eigenvalue and scales by the tensor factor. Preparation errors are
elected before collective trial evaluation. Partition work estimates now use
metric density/volume and edge demand without q/J measurements. The known atlas92
redundant-feature assertion led to a structural canonical-piece count check rather
than another geometric proof. Build94 evidence below supersedes that implementation checkpoint.

### Workflow evidence must include actual M6 continuation

The unchanged run_m6.py case is a steady one-event early diagnosis; its acceptance
parser explicitly does not certify transfer/reload. Retain that comparable run,
then derive a separate M6 BDF2 campaign with unchanged mesh/geometry/metric gates:
TIME_DOMAIN=YES, TIME_MARCHING=DUAL_TIME_STEPPING-2ND_ORDER, TIME_STEP=.001,
TIME_ITER=3, INNER_ITER=2, ADAP_FREQ=1; remove steady ADAP_SUBITER/ADAP_FLOW_ITER
controls as required by CConfig. Keep ADAP_SIZES=(500000), CONSERVATIVE and the
same reference. Require both event groups on all six ranks, current and prior-time
conservation/positivity, written mesh/sidecar and matching saved history, then an
actual restart continuation. The independent audit now supports tetrahedral P1 histories, but has no accepted
M6 inputs. Reuse the driver/output/transfer facilities and require its actual
3D audit before claiming the full milestone. No such M6
workflow is accepted at this checkpoint.

Build94 actual workflows are terminal with unchanged pins. M6 Euler371.127s:
303359 input tetrahedra; two private edits per rank (12 global work), then an
internal atlas storage refusal on rank4, requested721130288 versus714987896.
Native configured limit remains12288MiB/rank; sampled whole process tree about
3.44GB, largest sampled rank631MB. No work/time exhaustion or accepted mesh.
Import203.69s; short full-stack profile finds86% of sampled cycles in
ImportGeometry→FindFace→ContainsEntityFace→OrientedFace/Bezier Blossom.
Small mapped94 FAIL159.373s: flat Euler/RANS transfer/reload pass; curved variants
295 splits/238 collapses, one quality stage,20 optimizer moves/8 insertions/16 swaps,
609 private edits total,180 cells and33 still invalid at the same67207168 geometry
work cap. First collapse107576←103688 fresh replay builds valid cells with35649
work and1.88e-16 surface bound; actual refusal is size, not geometric impossibility.
Next connected fixes reuse existing exact accepted-face associations for unchanged
entity faces (changed faces retain geometry checks), fix fixture backend setup and
identify the exact geometry operation/scratch in storage refusals. No cap raised.

### Current-stage target and memory contracts, build96

A new supplied stage field is authoritative even when input cells already carry
attributes from an earlier adaptation. Exact matching source cells initialize
attributes directly; unmatched cells use bounded collective point sampling once
at stage entry, including empty-rank participation. NativeRemesher supplies current
working snapshots between construction, normalization and AdaptMetric, so corrected
fields are not overwritten by stale event-original data. Operators still transport
current nodal values through complete cavities; no interpolation ancestry persists.
The source field must cover unmatched input nodes; absent data rejects collectively.

M6 build95 identifies QuerySurfaceUnion's actual allocation refusal:14.03MB scratch
against7.89MB remaining in an incidental16MB reservation, while configured per-rank
memory is12GB and sampled process-tree RSS is about3.44GB total. Geometry query
arrays now admit against the existing configured memory pool with their actual
caller/scratch requests. Integer workspace and arithmetic work bounds are unchanged.
This fixes memory-pool accounting after the representation change; it does not
raise the configured cap or address quality stagnation by increasing arithmetic.
All geometry byte peaks feed the native admission report. Build96 compiles; focused tests execute122 cases with119 passing. Fresh-target
bootstrap and current normalization pass. M6 passes the old memory refusal and
fails the numerical surface-union calculation described below. Complete curved/
M6/BL workflow acceptance remains open.

### Geometry source paths and retained SU2 responsibilities

Source inspection for the geometry decision includes SCOREC ma/maSnap.cc676–704
(classified midpoint/parameters),749–755(model evaluation),856–873(target);
ma/maSnapper.cc54–68(local cavity),590–629(rollback and topology operations).
MMG mmg3d1.c363–410/490–583 checks local normals/tangents and Bézier deviation
against hausd²; colver_3d.c527/785–886 checks the prospective boundary after
collapse; movpt_3d.c480–506/1320–1407 evaluates prospective adjacent faces.
These are local model/deviation operations, not per-operation whole-original-model
certification. The new local guidance follows this division of responsibilities.
MMG's current-mesh Bézier fit is not substituted for the immutable SU2 geometry.

| Mechanism | Decision and provenance |
| --- | --- |
| Nodal size interpolation and classified complete cavities | Established foundation, implemented in existing SU2 operators. Convex SPD extension is an explicit SU2 policy, not a user requirement. |
| Persistent smooth rational covers/edge paths | Agent-selected legacy mechanism; entity mode discards face covers and retains genuine feature runs only. No history-dependent smooth path is required by this design. Legacy decode remains for compatibility. |
| Per-candidate whole-surface certificates | Removed from selection/operator guidance; retain the existing independent final geometry gate pending accepted workflow evidence. |
| Candidate scheduling and search/work limits | SU2 implementation choices, not geometry feasibility proofs. Bound fresh work and preserve first failure; no cap raised by this change. |
| Owner/version publication | Required SU2 integration responsibility: validate complete incident cavities, elect conflicts and publish associations/cells atomically. Existing Directory reused. |
| CFD transfer/history/restart | Independent SU2 conservative projection and persistence responsibility; nodal metric transport does not replace or certify it. |

### Local geometry guidance, build99 integration

M6 build96 moved beyond the incidental memory refusal to10 private edits/rank,
then failed because QuerySurfaceUnion exhausted exact dyadic resolution. This
does not establish operation infeasibility. In the selected classified-cavity
design, candidate faces use immutable geometry projection samples and existing
curvature chord-height guidance, with no h0 dependence. Same-original-facet
faces reuse the existing cheap cubic bound. Cross-facet faces no longer build a
whole model tessellation per candidate. The current final two-sided global
geometry gate is retained: local guidance never claims a whole-face certificate.
Body/sheet restrictions filter closest-point search before updating distances;
failed queries retain actual attempted work rather than the whole reservation.
No new persisted representation, external executable or format infrastructure.
Affected callers: selection, collapse constraints, local optimizer and regional
construction; publication/source/persistence continue using existing associations.
This is unvalidated implementation until actual curved and M6 workflows run.

Build97 was stopped after a missing internal stats argument compile error;
build98 was stopped for a source-review scope correction before validation.
Neither is a completed build despite Ninja signal exit0. Build99 is the owned
one-worker build; source pins remain fixed throughout it. Surface-distance
guidance includes the boundary of the requested sheet; actual node movement
retains its own genuine-feature restrictions.

Build99 passes1276.434s with1295 unchanged pins. Nodal122/122 and backend11/11
(414 assertions) pass. Compiled selection audit covers25 cases, with311 serial,
288 MPI,74 atlas and122 nodal tests selected. The broad atlas batch was deliberately
stopped442.217s; its legacy prism expectations fail and its repeated-event receipts
are partial, not a completed suite. Current import seeds nodal metrics even with
no explicit tensor (identity); demanding a metric shell or a narrow-gap shell
rejection is obsolete. Replacing those tests must preserve state/reference/owner
coherence and independently test real body separation; absence of a shell refusal
does not establish body safety. At that historical checkpoint, six-rank M6 build99 was active; its terminal
result follows below. No accepted hard-target workflow was established.

### Coarsen/refine cycles, build100 integration

M6 build99 was deliberately stopped after1369.419s with unchanged source pins;
all six ranks are terminal. Four complete split-only passes grew current cells
315809→354684 and cumulative splits3166→12673, with zero collapses. Size-bad
incidence112523→116291 and geometry-bad4115→5396 grew; max length10.815→10.631
stalled after the second pass. These do not prove no admissible collapse exists:
source policy repeated any productive split generation before even trying collapse.
SCOREC ma.cc instead executes coarsen/refine/snap in each iteration.

Build100 adopts coarsen-first complete passes alternating direction, entering
improvement after an unproductive pair (superseded by build108 staging above). Blocked support is still invalidated
only through existing dependency/publication facilities. No new cap, fixture or
physics dispatch, geometry representation or fallback. The opposing-demand
regression replaces its agent-policy assertion demanding refinement quiescence
before collapse with first-collapse/alternating-pass checks. Every original
validity/quality/height/input-atomicity assertion remains. Actual hard workflow
closure must qualify this policy; scheduling alone is not mesh acceptance.

One3s profile sampled792 user-cycle events with no lost samples; flat leaf costs
include interval closest-point operations and MPI progress. Detailed stacks emit
DWARF warnings. It identifies certified projection inside operators as remaining
cost, not a full-run percentage or reason to increase arithmetic limits. Existing
PNTriangleSurface::Minimize provides bounded Gauss-Newton/backtracking/simplex
edge KKT projection that may be reusable if numerical model projection must
replace unnecessary distance certification; no new projection variant implemented.

Build100 passes291.483s with unchanged1295 source pins; opposing-demand3 cases
pass5.987s. Actual mapped workflow is running with fresh failure capture. No
accepted hard-target capability follows from those focused cases. Future CAD
remains one optional Open CASCADE backend: STEP first, faceted STL then IGES;
STL is not inferred to be an exact smooth CAD surface. No format infrastructure
is added before this existing interface is qualified in the working workflow.

### Integrated projection checkpoint, build102

Existing Minimize/patch/derivative arithmetic is shared, not reimplemented. The
captured query(0,.805,.5), hint3, had a converged foot at1.24e-16 while candidate5
failed. A two-triangle reproduction exposed inward reentry on rounding noise
before the existing KKT stationarity tolerance was consulted; consulting it first
fixes both orientations without changing tolerances. Search selects converged
eligible feet, uses conservative [0,upper] bounds, stops at requested accuracy,
and refuses if none exist. Certified ClosestCurved and final Both remain unchanged.

Build102 passes72.187s/1295 unchanged pins;13 backend cases and all10 DC18 geometry
groups pass. Current nodal122 cases pass. Compiled26 selections include313 serial,
290 MPI,76 atlas and13 backend cases. This is supporting evidence only. Actual
curved workflow enters improvement, then exhausts geometry work with unmet final
gates (58.021s). M6 rejects292.221s on no admissible converged projection, facet11053,
after70 private edits. Raw receipts and captured queries separate these blockers
from mesh impossibility. Current [projection findings](ADAPTATION_PROJECTION_FINDINGS.md)
record the dominant exact association/feature work and remaining admission question.
No cap raised, no accepted hard-target transfer/restart/BL capability claimed.

Focused current-field122 cases pass10.036s and six-rank coarsen/refine3 cases
pass5.906s, with pins unchanged. These do not supersede the failed real curved/M6
workflows or the pending obsolete metric-shell test maintenance. All owned job
handles/ranks are terminal at this checkpoint.

## Captured boundary reconstruction decision — 2026-10-29

The captured139-cell smaller workflow stalls with cell122900: four vertices on
one almost planar prescribed wall sheet, roundoff-scale positive volume and
unacceptable q/J. Boundary mobility is a user requirement; keeping its two wall
triangles fixed is not. Fresh native collapses reject exact negative cells without
work exhaustion. Independent complete-ring enumeration rejects all14 interior
edge-removal triangulations. Additional midpoint-collapse and numerical cone-kernel
analyses supply no acceptable fixed-skin repair; neither search failure nor an LP
zero radius proves general impossibility. Receipts are in staged57/serial_replay115.

A concrete alternative changes the working boundary diagonal over the complete
six-cell incident ring. Retire cell122900, promote its two previously internal
faces on the same original wall entity, and retain the other five cells. Exact
positive cells and an oriented two-manifold global skin survive in the captured
mesh. Minimum q/J become .24484/.11255, no volume edge is added, and size debt falls.
This is a proposal, not accepted native reconstruction: genuine feature chains,
body identity, original cubic geometry tolerance, nonlocal contact and owner closure
still require existing native checks. No declared feature may be retired.

The selected SCOREC implementation's boundary EdgeSwap3D delegates to
EdgeSwap2D::checkTopo, which rejects an already existing opposite diagonal.
MMG5_chkswpbdy likewise rejects a one-tet boundary component. Thus this elementary
boundary shelling is a proposed native extension to classified cavities, not a
facility falsely attributed to those implementations or a user-prescribed operator.
Do not silently relax their topology tests. MMG's preprocessing of multi-boundary-
face tetrahedra is another established design choice, not an imposed requirement.

Before implementation, replay the proposed replacement through existing
ValidatePrescribedInterface, RebuildFeatureTraces, forward associations,
ContainsFace and complete SurfaceError checks with fresh work. If admitted, the
small connected change belongs in the shared boundary kernel and the optimizer's
quality phase: infer complete current incident-edge closure, use Finish for original
entity/feature associations, and publish fresh neighbor cell identities and physical
faces through the existing atomic owner protocol. Source reconstruction and restart
use those same published cells and current associations; no new archive schema,
persistent path, projector, case dispatch or work formula is justified.

Construction remains validity-only. Quality-phase proposals use existing measured
shape/height and metric admission; every final mesh, geometry, quality, wall-height,
conservative transfer/history and solver restart gate remains. Acceptance requires
the captured smaller whole workflow, neighboring/overlapping publication and
repeated events, then early M6 under comparable gates. A passing proposal alone
cannot close DC-17 or DC-18.


## Geometry permissions and quality-stage coupling — builds121/122

An inherited sampled-curvature sizing calculation was used as a hard geometry
permission/current-mesh demand. Probe120 measures.0604383 through that surrogate,
but.00726343 through existing original-surface validated-face error, below requested
.01. Full collapse and ContainsFace pass. Build121 deletes the custom sampler and
reuses SurfaceError/SurfaceErrorForValidatedFace for the shared entry point. Original
whole-surface coverage remains separate and mandatory. No projector fallback, new
proof representation or tolerance change is justified.

Actual build121 workflow still reaches4096 work after33 phase pairs: existing
quality smoothing suppresses geometry constraints whenever global shape admission
is ValidityOnly. That global mode must remain for validity-first geometric builders;
it is not the quality-stage geometry policy. Ten complete quality phases increase
actual geometry debt, one14.76-fold, undoing previous construction work.

Activate the existing geometry-excess constraint while selecting boundary smoothing
candidates, independent of global shape mode. At shared ordinary Repair admission,
retain the existing rule against increasing geometry excess. Real requested-height
GeometryRepair remains explicit. Geometry excess is zero for faces within the
requested tolerance, so boundaries remain free to redistribute inside it. Nonincrease
of this area-weighted excess is an agent-selected quality merit responding to the
capture, not a new user requirement. Construction remains validity-only, every
final acceptance gate stays unchanged, and no work cap is raised. A subsequent
failure must distinguish absent admissible candidates from scheduling/cost before
further algorithm changes.
