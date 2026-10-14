# Goal 2 joint reconstruction WIP — 2026-10-13

Repository `/media/rausa/4TB/SU2_Versions/SU2_NativeIntegrated`; branch
`codex/native-3d-curved-local`, previous published `68b24b82eccd9f420f4990d814bbedb530c2a10d`.
Goal ACTIVE; all four full requirements PENDING. Connected implementation adds
projected physical-edge split plus one interior point and complete skin-constrained
reconstruction, using shared projection/surface-child validation. A frozen actual
old-centroid tensor guides bounded metric-frame placement only; all final actual
centroids/edges are checked against the unchanged original/declared source.
Complete neighbouring-face dependencies are imported, version/owner checked and
re-reserved; 12 trials/64 old/128 new, global work/memory/time ceilings remain.
Every extra joint trial counts as work. Curved node IDs reserve two slots; planar
IDs unchanged. Missing/protocol/storage failures remain collective vetoes. Recovery
retries reopened geometry queues as well as oversized metric edges. Existing global
curved serialization, clearance, atomic CFD rejection and conservative rebuild stay.

joint_replay_v1: saved v7 obstruction admits six-old/twelve-new coupled proposal,
48% local geometric deficit reduction, bounded pattern q0.224286/J0.065229.
Native saved-geometry regression passes frozen0.5I metric, geometry progress,
exact pairwise contact/no-overlap and rejection rollback. The small generated
fixture includes its original saved-source hash. This is a local operator proof,
not full arbitrary P1/CFD/MPI/goal qualification.

joint_runtime_v1 terminal FAIL test missing include (production compiled), retained.
joint_runtime_v2/v3 terminal FAIL4/5 serial selected cases PASS; tilted cylinder
still stagnates. joint_runtime_v4 terminal FAIL4/5 PASS; limiting quality/J-ratio
face growth increases private edits to86 but does not complete the case. Last
rejection feature3:1733 geometric error non-improvement; accepted CFD state retained.
No acceptance/source tolerance was relaxed. Full production/member/binary pins and
saved grids/states/failures retained. No broad current-source regression yet.

joint_replay_v2 session29240 TERMINAL: no admissible one-interior-point proposal
in792 bounded trials on the latest v3 rejected geometry. Owned nice19 process moved
fromCPU0 toCPU2 due contention; adjustment record retained, no timing claim. Exact
papers folder found `/media/rausa/4TB/SU2_Versions/Prove/Papers`; read Loseille/Alauzet/
Menier2017 CAD paper sections2.3–2.5 and local repair/2D movement code. An overly broad
owned file search was identified and stopped; no unrelated process touched.

LIVE controller75064: `integration_evidence/native_3d_curved_v1/joint_mpi_audit_v1/run.py`.
Current v4 source/binary pinned, sequential MPI2/4 on oneCPU/nice19/threads1, no new
build beyond no-op integrity check. It continues observations after matrix failures;
status FAIL_MATRIX must not be presented as green. Revalidate its handle/receipt;
do not launch another heavy job or change pinned source while live. Case folder
currently `joint_mpi_audit_v1/mpi2`; MPI4 follows. New test folders already announced.

Next substantial work: coordinated surface retriangulation and/or multiple interior
points to escape the recorded full-path blockage; stop tuning single-point growth
order or raising caps. Preserve failed matrix. Qualify current-source MPI/rejection/
accepted-state audits, then the meaningful broad integration milestone and genuine
solved Euler/resolved nonuniform control. Original full Goal2scope intact; no cluster/
performance/CAD/production3DBL/full physical unsteady/force-convergence claim.
Report JOINT_RECONSTRUCTION_WIP.md. AGENTS.md permanent batching cadence applies.
ForeignClusterResults/dependencies/AdapNoExt untouched.

# Goal 2 primitive runtime WIP — 2026-10-12

Working repository `/media/rausa/4TB/SU2_Versions/SU2_NativeIntegrated`, branch
`codex/native-3d-curved-local`; previous published commit `2d00b57ffabd78ce05f4350a71fb6b210d93c9a0`.
Goal ACTIVE, all four full requirements PENDING. This connected batch adds finite
arbitrary-axis cylinders, disk cap/rim authority, certified complete primitive
components, shared cylindrical source/projection geometry and axis-aware v3
reference persistence (v1/v2 retained). Geometry selection retains alternatives;
finite-triangle sphere error is tighter where the plane foot is outside. Geometry-only
repairs require actual geometry progress plus strict shape/sliver/nonincreasing
metric excess. Oversized curved repair candidates are skipped using the existing
batch dependency protocol; storage/protocol/missing-feature failures still abort.
Cavity 64/replacement 128/work 4096 limits and accepted-state safeguards unchanged.

Latest focused controller `primitive_runtime_v7/run.py`, session 52797, TERMINAL
FAIL: build PASS, serial 3/4 cases PASS; primitive matrix fails on tilted cylinder.
Sphere and axis-aligned cylinder complete both actual CFD replacement cycles,
BDF/SU2 restart and SU2/CGNS readback. Tilted cylinder accepts 77 private edits,
then operator-stagnates; rejected replay has 121 cells/62 nodes. MPI2/4 were NOTRUN
because serial failed. No broader regression for this changed production batch.
All v1-v7 failures/source/build/member/binary pins and relevant grids/states remain.
The v5 estimated-gain rounding experiment did not fix rotation and was removed.

Independent corrected auditor `primitive_audit_v1/audit_primitives_final.py`,
session 80275 TERMINAL PASS, produces `primitive_runtime_v7_audit.json` with status
PASS_COMPLETED_LIFECYCLES_ONLY, explicitly listing failed/unrun variants. Exact
volume/closure/no interior overlap, geometry/source/metric, conservative actual
current/BDF integrals, positivity, transported metric residual 0 and real restart/
writer signatures pass for the two completed serial variants. Initial auditor
used box marker defaults incorrectly; retained source/AUDIT_CORRECTION explain the
existing marker-parameter fix. No runtime source or gate changed for that audit.
No owned heavy job remains live. Foreign ClusterResults/dependencies/AdapNoExt untouched.

Inspect cases in
`integration_evidence/native_3d_curved_v1/primitive_runtime_v7/mpi1/`:
`sphere_axis0_m1_mesh_00001.su2` (26 cells/18 nodes),
`cylinder_axis0_m1_mesh_00001.su2` (42 cells/32 nodes), and both second-event files.
Rejected tilted grid exported separately at
`primitive_audit_v1/primitive_runtime_v7/mpi1/cylinder_axis1_m1_s0_rejected.su2`.
Frozen geometry targets remain 0.22R sphere/0.04R cylinder, sensor0.5I, q>=0.20,
J>=0.05, metric edge<=1.8. These are coarse prescribed-metric controls, not solved
primitive CFD or tight-geometry proof. Current-source MPI/M<N, component/feature
negative controls, broad regression and resolved nonuniform/shock remain pending.

Next substantial step: exact replay of the saved obstruction and coordinated
boundary/transition cavity reconstruction with off-edge/interior placement under
unchanged dependency/replacement bounds. Do not continue rounding priorities or
blindly increase caps. Then qualify the connected result, MPI and independent
accepted-grid/state audits; retain full Goal 2 scope. Permanent cadence AGENTS.md:
focused checks while developing, broad suite at meaningful integration milestones.
Report PRIMITIVE_RUNTIME_INTEGRATION.md and full requirement audit remain incomplete.

# Goal2 curved conservative and solvedEuler lifecycle — 2026-10-12

Working repo /media/rausa/4TB/SU2_Versions/SU2_NativeIntegrated; branch
codex/native-3d-curved-local, previouspublished dbd44516cc. GoalACTIVE/all4fullgates
pending. New single-file lifecycle controls pass actual curved growth/shrink,
threeconservative refine/coarsen events, bothBDF histories through CGNS restart,
actual SU2/CGNS readback, and twoactual solved3DEuler pressure-Hessian metric events
with SU2/BDF restart and solver continuation. No production policy/gate changed.
Source test changes oneobject; libraries/archive members unchanged from previous
100case/rank MPI1/2/4 regression. Permanent cadence in AGENTS.md: no redundant broad
rerun for unchanged production. Focused curved_lifecycle_v1 PASS2cases/rank
MPI1/2/4; independent_lifecycle_validation.json PASS, source/build/binary/67other
object/production member pins verified. Controller73168/auditor63150 terminalPASS;
no owned heavyjob live. ForeignClusterResults/dependencies/AdapNoExt untouched.

Inspect curved_lifecycle_v1/mpi1/solved_bump_m1_mesh_00001.su2 and_00002.su2,
and curved_lifecycle_grow_m1_mesh_00001.cgns, plusMPI2/4/M<N. Serialgrowth8->48->8,
shrink8->48->10; new/lostvolume~0.00020833. Actual accepted-state integrals match
conservative targets, allthree stored states positive, all15restart values match.
Solved subsonicMach0.5/slip-bump pressure yields nonuniform anisotropic changing
tensors, actual event meshes37cells/21then22points. Minimum savedserialq~0.438688,
maxedge~1.792384, metrictransport residual0. This is a small solved lifecycle, not
resolvedshock/convergedphysicalunsteady/forces proof. Curvedcommits stillserialized.

Report CURVED_EULER_LIFECYCLE.md; full completion auditINCOMPLETE. Initially prepared
independent auditor hadoneunmatchedparenthesis; immutable original retained,
audit_lifecycle_final.py corrected, noCFD/source/binary/gate changes. Actual cfgs,
grids/refs/source/state/projection-ledger/restarts/logs retained. Next substantial
work: sphere/cylinder runtime and fullmixed/feature geometry, then suitably resolved
nonuniform/shock controls. Keep original fullGoal2scope; no cluster/performance/CAD/
production3DBL/affordability claim. Earlier notices are historical.

# Goal2 connected curved runtime integration — 2026-10-12

Working folder /media/rausa/4TB/SU2_Versions/SU2_NativeIntegrated; branch
codex/native-3d-curved-local. GoalACTIVE; all4full integrated gates pending.
Blanket analytic refusal replaced by MPI-consistent positive frozen surface
ceiling; geometry drives selection/progress while strict q/J/length remain.
Projected actual-query source imports, projection-owned movement and exact indexed
current-mesh overlap/conforming-contact checks precede atomic publication. Curved
commits serialized until concurrent-displacement collision reservations exist;
planar batching retained. No performance/scaling claim or cluster work.

curved_runtime_regression_v2 PASS:100selected cases/rank MPI1/2/4, full68-object
source/archive/member/binary pins, independent accepted-grid/state/source/volume/
sidecar auditPASS. Actual initial analytic bump sidecar loads;6->8CFD driver mesh
replacement and constant current/BDF transfer pass allM=1..N. Inspect
integration_evidence/native_3d_curved_v1/curved_runtime_regression_v2/mpi1/curved_runtime_m1_mesh_00001.su2
plusMPI2/4/M<N files. Volume1.0002083333333334 approaches analytic1.0002777777777778;
sampled wall error0.00015625<0.003 complements conservative whole-face bounds.
Sensor prescribed0.5I; not solvedCFD-derived metric or physical unsteady proof.
No q/J/length/geometry/BL/fade/NOISE0/transfer/MMG gate weakened.

Connected twelve-file implementation and controls, source snapshots and earlier
owned-build interruption/compile failure retained. First broad99/100 failure was
an obsolete diagnostic assertion, now corrected; failed original receipt remains.
All original artifacts hash-verified directly or in compact planar payload archives;
actual curved grids/sources/states/sidecars stay inspectable. Report
CURVED_RUNTIME_INTEGRATION.md; full audit remainsINCOMPLETE. Controllers94180/78676
terminalPASS, no owned heavy job live. ForeignClusterResults/dependencies/AdapNoExt
untouched. Permanent cadence in AGENTS.md: connected code, focused checks, broad
regression at meaningful integration milestones, regularly push reviewable work.

Next substantial work: repeated curved conservative/current+BDF/restart/CGNS and
real solvedEuler-derived metric/remesh/continue cycles. Sphere runtime/cylinder and
full geometry/feature coverage remain pending. Preserve full objective; do not close
Goal2 from this first accepted prescribed-sensor bump event. Older notices below
are historical.

# Goal2 connected curved runtime — implementation in progress, 2026-10-12

Working folder /media/rausa/4TB/SU2_Versions/SU2_NativeIntegrated, branch
codex/native-3d-curved-local, previous published9e7bcac255. Full goalACTIVE/all4gates
pending. Connected production changes now replace blanket analytic refusal with
positive frozen surface tolerance, geometry-driven selection/progress, projected
actual centroid/edge imports, movement-owned projection and exact indexed current-
mesh contact/overlap checks before publication. Curved commits are serialized;
planar batching retained. No performance qualification. Twelve production/test
files changed together; see CURVED_RUNTIME_INTEGRATION.md.

Focused curved_runtime_v3 PASS on MPI1/2/4: actual initial sidecar loads,6->8CFD
replacement, constant current/BDF transfer, actual SU2 writer/reference reload.
Independent runtime auditPASS, volume moves toward analytic bump authority.
Prescribed0.5I sensor, not solvedCFD-derived metric. Inspect
integration_evidence/native_3d_curved_v1/curved_runtime_v3/mpi1/curved_runtime_m1_mesh_00001.su2
andMPI2/4/M<N equivalents. First broaderMPI1 selection99/100 passed; only failure
is obsolete blanket-refusal text assertion, now corrected to frozen-tolerance
refusal. Production code unchanged since focused qualification. Prior controlled
build stop/compile failure and broad failed receipt/log/source patches retained.

Owned controller94180 is the sole live heavy job at this writing, in
curved_runtime_regression_v2; poll its handle/terminal receipt before modifying
pinned source or restarting. It incrementally rebuilds the changed test object,
then runs100selected cases on localMPI1/2/4, nice19/oneCPU/threads1. Next: independent
accepted-grid audit for broad receipt, final source/artifact proof, update report/
findings/completion audit and push this substantial connected implementation.
Then prioritize actual solvedEuler metric cycles, repeated curved conservative/
restart/CGNS and remaining geometry (including cylinder), rather than another
sequence of isolated micro-checkpoints. ForeignClusterResults/dependencies/
AdapNoExt untouched. Permanent implementation cadence in AGENTS.md.
Earlier notices below are historical.

# Goal2 automatic curved sensor-source integration — 2026-10-12

Working folder /media/rausa/4TB/SU2_Versions/SU2_NativeIntegrated; branch
codex/native-3d-curved-local. GoalACTIVE; all4integrated gates pending. Actual CFD
boundary import now produces bounded sensor-only bump columns/sphere frusta before
freeze. Original physical P1 retains priority; original boundary tensors are copied
exactly and new records migrate with their CFD parent for M=N/M<N. Engine/reader
association is wired; private projected-query footprint/component handling and
geometric completion/clearance still block curved runtime publication. Source
fixtures use prescribed arrays on actual CFD geometry, not solved CFD-derived
metrics. No 2DBL/fade/NOISE0/MMG/solution-transfer/acceptance policy changed.

generated_sensor_v2 PASS:97cases/rank MPI1/2/4/full68-object driver. Independent
source auditPASS: exact positive volumes/no-interior-overlap, shared coordinates/
tensors, parent boundary-face provenance and physical MPI consistency. Private
curved operator shape/closure auditsPASS. Source SU2 envelopes/JSON/native references
are directly inspectable in generated_sensor_v2/mpi1 (andmpi2/4); these are not
adapted CFD grids. V1 mistaken numeric facet-ID invariance assumption is preserved
with source/log/receipt and corrected physical-provenance audit. V2 also corrects
parent-routing map storage admission after moving the original cell container.
Every original controller artifact is hash-verified directly or in a compact
planar payload archive. Report GENERATED_SENSOR_CHECKPOINT.md; source/member/binary
pins and publication_integrity.json retained. Controllers2538/91704 terminalPASS;
no owned heavy job remains live. ForeignClusterResults/dependencies/AdapNoExt
untouched. No cluster/performance qualification.

Next substantial step: geometry-driven refinement/completion, actual projected-
candidate source routing and nonlocal clearance together on one end-to-end curved
runtime case, then accepted curvedMPI/solution/history/output/restart and actual
Euler metric cycles. Cylinder/mixed geometry remain gaps. Do not close Goal2 from
these import/operator controls. Permanent user cadence in AGENTS.md: group
connected implementation work, focused development checks, broad regressions at
meaningful integration milestones rather than every small edit/checkpoint.
Earlier notices below are historical.

# Permanent implementation cadence — user instruction, 2026-10-12

See AGENTS.md. Group connected changes into substantial end-to-end implementation
steps. Use focused checks during development; run broad regressions at meaningful
integration milestones, not after every small edit/checkpoint. Keep necessary
correctness gates and regularly push clearly scoped, reviewable work. This rule
persists across context compaction and future sessions.

# Goal2 declared exterior sensor field checkpoint — 2026-10-12

Working folder /media/rausa/4TB/SU2_Versions/SU2_NativeIntegrated; branch
codex/native-3d-curved-local. GoalACTIVE; all4full integrated gates pending.
Original physical P1 now has priority over explicitly associated frozen exterior
cells at queries and exact rational edge intervals. Tags travel through MPI source
imports; wrong components/gaps/ambiguous same-priority intervals reject. Original
thin demands and coincident-cut nonzero widths are preserved. Composition still
uses actual queries/cache hits; composed-target edge audits still reject without
geometric integration. All records remain validated; native field scratch models
include bounded layered trace/cache/index storage. No 2DBL/fade/noise/MMG/transfer
policy change. Default callers exclude tagged exterior coverage.

layered_sensor_v2/validation.json PASS:95cases/rank MPI1/2/4 with full68-object
driver; independent shape/closure/no-overlap audit alsoPASS. New source-tag test
is actual MPI import; curved split/field witness uses a manually prescribed finite
shell. Inspect layered_sensor_v2/mpi1/layered_sphere_split.su2 and matchingJSON,
plusMPI2/4. Not solvedCFD/curvedMPIpublication/history transport or production
coverage-generation proof. Firstv1 failed on a real fixture segment gap; exact
source/logs retained and gapped geometry remains a passing negative control.
V2controller73422 terminalPASS; no owned heavy job remains live. Incidental planar
payloads are hash-verified in archives. Source/proof pins and full limitations in
LAYERED_SENSOR_CHECKPOINT.md. ForeignClusterResults/dependencies/AdapNoExt untouched.

Next big integration: actual CFD-derived exterior coverage generation/admission
before freeze and actual private/remote query footprint/component routing, then
geometric error/clearance completion and actual curvedMPI/CFD/restart. Source layer
is implemented, producer/integration remains missing. Cylinder/mixed geometry and
nonconvex chart unions remain gaps. No cluster/performance gate or claim.
Earlier notices below are historical; do not close Goal2 from these prototypes.

# Goal2 joint curved geometry checkpoint — 2026-10-12

Working folder /media/rausa/4TB/SU2_Versions/SU2_NativeIntegrated; branch
codex/native-3d-curved-local. GoalACTIVE, all4integrated gates still pending.
Compatible supplied bump facets now share certified connected-convex XY charts;
markers, primitive definitions and disconnected components remain distinct.
Off-edge surface-cavity insertion, projected movement across original seams and
changed-diagonal collapse are implemented with positive inherited cell orientation,
whole-face chart checks and atomic rollback. Geometry metadata/v2 persistence
reconstructs the canonical union. Original raw authority/default planar behavior
and sensor/BL/fade/noise/MMG/transfer policies are preserved. Runtime curved
adaptation still refuses pending coverage/final geometry checks.

joint_bump_geometry_v1/validation.json PASS:90cases/rank MPI1/2/4 with full68-object
driver; independent private_shape_validation.json also PASS. Actual saved inserted,
moved and coarsened grids in joint_bump_geometry_v1/mpi1, mpi2 andmpi4. This is
private operator and collective reference proof, not curved distributed adaptation,
solution transport or actual solved3DEuler proof. Incidental planar regression
payloads are hash-verified in an archive; current private grids remain inspectable.
Controller31846 terminalPASS; no owned heavy job live. ForeignClusterResults,
AdapNoExt and dependencies untouched. See JOINT_BUMP_GEOMETRY_CHECKPOINT.md.

Next: integrate geometric completion/clearance and bounded frozen source coverage
with consistent exterior edge integrals; then actual curved MPI/state/output/restart
and small realEuler cycles. Cylinder, mixed curved/planar expansion and nonconvex
chart unions remain gaps. Do not close Goal2 from these operator prototypes.
Earlier notices below are historical; no cluster/performance qualification.

# Goal2 analytic geometry checkpoint — 2026-10-12

Working folder /media/rausa/4TB/SU2_Versions/SU2_NativeIntegrated; branch
codex/native-3d-curved-local. GoalACTIVE, all4integrated completion gates pending.
Analytic sphere/bump finite authority, coupled projected complete-edge-star split
and inverse, inherited-connectivity inversion rejection and version2 persistence
are implemented. Default planar rules/v1 references retained. Final local receipt
native_3d_curved_v1/analytic_geometry_v3/validation.json PASS: full68-object driver,
86cases/rank MPI1/2/4, connected planar bridge/runtime/default and2Dfield controls.
Independent private_shape_validation.json PASS: identity shape/sliver gates,
exact rational no-interior-overlap and signed-skin/cell-volume closure on actual
saved original/split/coarsened sphere/bump private proposals. Inspectable SU2/JSON
in analytic_geometry_v3/mpi1 (andmpi2/mpi4). These are not solved CFD grids.

Mixed curved/planar finite-patch expansion remains unsupported and rejects;
initial failed experiment/source retained in analytic_geometry_v1. V2 preliminary
source/evidence also retained; V3 fixes acute-corner tolerance amplification by
using finite arc/edge distances. No loosening of geometry/quality/field gates.
Runtime explicitly refuses analytic authority pending integrated donor extension
and geometry completion. Existing2D/3D sensors/BL/fade/NOISE0/MMG/transfer unchanged.
No owned build/test controller remains live: v3session33629 and shapeaudit42293
terminalPASS. AdapNoExt, foreignClusterResults and dependencies untouched.

Full stage scope, evidence and remaining steps:
integration_evidence/native_3d_curved_v1/ANALYTIC_GEOMETRY_CHECKPOINT.md.
Next main step remains complete coupled geometry: chart unions/feature constraints,
curved operations and geometry-driven completion/clearance, supported by explicit
bounded triangular sensor extension and exterior edge integration. Point-query
fallback alone is insufficient. Cylinder and actual curved MPI/CFD/restart gates
remain pending. Keep one owned heavy job, announce folders, localMPI<=4, push
checkpoints; no cluster/performance qualification. Earlier notices are historical.

# Goal2 activated: local curved-boundary/Euler work

User cleared Goal1 and explicitly activated the revised second goal. Active branch
codex/native-3d-curved-local from23197eba1715f44d61c9ad3a070f61441dfb0560; working
repo /media/rausa/4TB/SU2_Versions/SU2_NativeIntegrated. Goal1 complete on its previous
branch. Goal2 excludes ALL cluster/performance qualification/large-wing campaigns.
Full current objective/gates in NATIVE_3D_DEVELOPMENT_GOALS.md. ClosedGoal1 plan
archived in native_3d_curved_v1/GOAL1_CLOSED_PLAN_23197eba17.md; its oldplan hash
refers to that completion snapshot, not subsequentGoal2 revisions.

First coverage study session86307 terminal PASS:
integration_evidence/native_3d_curved_v1/coverage_study_v1/validation.json.
OneCPU/nice19, threads1, full configured68-object driver; MPI1/2/4 selected
NativeCurvedCoverage3D+NativeMesh3D+NativeField2D. Production archives/members and
other67test objects unchanged. Manually prescribed three-tet exterior coverage
supports sphere-octant projection and sensor edge integration while preserving
original interior P1 exactly and rejecting uncovered queries. Existing2D bounded
associated-edge extension and wrong-component/distance controls also pass.
This is NOT production padding/curved remeshing/distributedgeometry/CFD or Goal2
completion. No owned controller remains live; new unit regression is qualified.

After user question traced existing2D: FieldPatch uses exact P1 first, separate
roundoff envelope, then nearest associated original boundary-edge trace within
ADAP_HAUSD; MPI import padding2*limit, query limit unchanged. Sensor interpolated
on old edge, BL composed at actual queryp. Native2D retains original polyline,
not unknown smooth curve.3Ddirection should reuse bounded associated-face trace
semantics with consistent exterior edge lengths; Queryfallback alone is incomplete.
Manual shell is a coverage control, not a commitment to productionghost tets.

Connected map: native_3d_curved_v1/IMPLEMENTATION_MAP.md. Current PlanarReference
and changed-skin checks protect exact faceted domains. Need explicit curved
geometry/features/component/provenance, coupled volume edit/clearance proof,
metric extension before freezing, MPI admission/persistence, existing3Dtransfer
new/lost-domain handling, and real smallEuler metric/remesh/continue/restart proof.
No delegation or extra heavy job; no source/quality/geometry relaxation, unsupported
3DBL still rejects. ForeignClusterResults/dependencies/AdapNoExt untouched.
Next: explicit bounded associated-face sensor trace and consistent exterior edge
integration, analytic geometry/component authority and first coupled curved edit.
Do not commit to ghost tets merely because the manually prescribed study passed.
Push this qualified study plus active plan; all4Goal2 integrated gates remain pending.
Previous Goal1 notices below are historical.

