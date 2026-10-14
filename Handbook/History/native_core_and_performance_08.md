## Native 3D movement/reconnection checkpoint — 2026-10-10

Working folder: /media/rausa/4TB/SU2_Versions/SU2_NativeIntegrated.
Branch codex/native-3d-core; predecessor92bff69fc1442f0e84eddc81edee85432e5bfccf.
Goal1 remains ACTIVE/full scope. MoveVertex replaces a point's complete star at an
exact facet-contained new position with a reserved replacement ID; positive
cell/face signs preserve the domain. Reconnect reuses fixed-skin Cone for2->3,
3->2 and4->4. Generic coordinate/metric guards remain unchanged. Original finite
triangle seams, including coplanar ones, remain protected; coplanar geometry-patch
association is still required for full original planar-surface freedom.

Final repair_operators_v2 PASS MPI1 (63 cases/3773 assertions), MPI2/4 (19 cases/rank),
and actual non-MPI private metric-validated movement/reconnection plus existing
refine/coarsen/rerefine controls. Ordinary/thin/sheared domains, M=N/M<N, actual
complete face/edge/vertex imports, reservation conflicts, original-P1 sensors,
private metric repair, rank-veto/storage rollback, publication/return all pass.
108 prescribed proposals+36 initial meshes independently pass exact global
embedding/volume/facet/marker/metric audits.702 imported donor records exact;
936 canonical vertex/interior queries with max normalized residual1.10e-16 and
byte-identical worker observations.216 intentional publication rejections preserve
both mesh and directory bytes. Earlier414 JSON/SU2 artifacts unchanged.

Inspect integration_evidence/native_3d_core_v1/repair_operators_v2/mpi{1,2,4}_repairs/
repair_{face,edge4,wall,interior}_{unit,thin8,shear4}_m*_s*.su2 and donor/mesh JSON.
All306 parent profiles close, including preparation/migration/communicator/worker/
return/test audit/export. Private costs, query subcosts and communication counters
are retained. Separate directory/donor byte models are not complete caller/RSS
qualification; overlapping single-patch proposals are not a scaling/balance test.

REPAIR_OPERATORS_CHECKPOINT.md and repair_summary.json give evidence/replay/limits.
V1 native MPI1 passed; its independent parser variable-shadowing failure and
30-frame saved-log re-audit are retained. V2 strengthens nodal sensors/interior
queries. MPI handle30089/non-MPI80660 terminalPASS; no owned job live.

Next: automatic bounded repair selection/fallback, actual shape/sliver stalls,
partial cavities/larger sources and exact tracing cost; coplanar geometry patches;
then weighted partition/runtime/CFD/CGNS and complete performance qualification.
The existing Refine/Coarsen selectors do not yet invoke these new repair operators.
Production3D remains rejected;2D/MMG/finer sensor/geometric BL/fade/noise intact.

## Native 3D general coarsening and repeated cycles — 2026-10-10

Working folder: /media/rausa/4TB/SU2_Versions/SU2_NativeIntegrated.
Branch codex/native-3d-core; predecessor badd73ba3a9261c4e9aa69d152a6f7ee3d6282c0.
Goal 1 remains ACTIVE/full scope. General vertex-star collapse now shares the
incremental MPI engine; no split-history requirement. Both short-edge directions
(<0.9), complete vertex dependencies, original-P1 sensors, unchanged q/J/L gates,
immutable incident finite facets, parent-owner routing and atomic rejection remain.
Movement/reconnection, larger sources, partition/runtime/CFD/CGNS remain unfinished.

Final general_coarsening_v2 PASS MPI1 (59 cases/3319 assertions), MPI2/4 (18 cases/rank),
and actual non-MPI 6->48->6->48 controls. Five ordinary/thin/sheared/spatial/turning
cases complete two refine/coarsen cycles per M=N/M<N subset. All120 event grids
plus15 initial grids pass independent global embedding/volume/facet/marker/metric
audits. 6746 imported donor records exact;1040 canonical P1 query samples with
max normalized residual9.02e-17. Every coarsening event returns6 cells; refinements
have40-48 cells. Prior54 box+90 anisotropic artifacts remain byte-identical.

Inspect integration_evidence/native_3d_core_v1/general_coarsening_v2/mpi{1,2,4}_cycles/
cycle_{box,thin8,shear4,spatial,turning}_m*_s*.su2 and original-sensor JSON.
s0/s2 refine, s1/s3 coarsen. Coarsening starts from the actual accepted fine mesh's
frozen original-P1 source, not the earlier six-cell source. Target changes only
between manufactured events (coarse sensors are one-quarter of fine tensors).

All340 parent profiles close including preparation, migration/worker communicator,
worker execution, return and test audit/export. Nested source/rejection controls
are labelled. Local private metric cost remains dominated by exact source-edge
tracing, not interpolation: box coarsening .1751/.2181 s nested trace;19 accepted,
9 rejected edits. Three OFF/ON non-MPI controls preserve geometry/work, with
coarsening medians .2149/.2096 s and unresolved timing overhead within variability.
Maximum observed rounds79/modeled bytes2374600, not size/RSS/scaling certificates.

GENERAL_COARSENING_CHECKPOINT.md and cycle_summary.json give evidence/replay/limits.
Earlier general_collapse_v1 wrong test expectation and general_coarsening_v1
wrapper-argument failure are retained with source pins/logs; v1 independent
constant-target re-audit passes15 frames. No owned jobs remain live after final
validation (MPI handle90541/non-MPI42734 terminalPASS).
Next: bounded movement/reconnection and exact source-trace cost, then larger source
controls/partition/runtime/output and full performance qualification. Production3D
remains rejected; 2D/MMG and geometric BL/finer-demand/fade/noise policies intact.

## Native 3D full anisotropic MPI refinement matrix — 2026-10-10

Working folder: /media/rausa/4TB/SU2_Versions/SU2_NativeIntegrated.
Branch codex/native-3d-core; predecessor e920aedd2780c7daa8c47ce1efb3bc311ada9267.
Goal 1 remains ACTIVE/full scope unchanged. Native production code is unchanged.

Final anisotropic_engine_v3 PASS MPI1 (55 cases/2936 assertions), MPI2/4 (17 cases/rank).
Five frozen targets: heights1e-4/1e-8, sheared thin domain, fixed-rotation spatial
sizing and changing-direction nodal metrics. N=M and M<N migrate original sensors
and mesh, refine completely, and return mesh to N. Thirty completed grids have
40-48 tetrahedra; spatial/turning M4 yield42/46 vs40/44 with fewer workers.
Each independently passes exact embedding/volume/facet/marker/metric audits.
260 canonical P1 samples have max normalized residual9.02e-17; transported nodal
sensors/coordinates are exact. Gates q>=.20/J>=.05/L<=1.8 remain unchanged.

View integration_evidence/native_3d_core_v1/anisotropic_engine_v3/mpi{1,2,4}_anisotropic/
aniso_{thin4,thin8,shear4,spatial,turning}_m*.su2 and *_initial.su2. All45 frames
have retained original sensor JSON. Earlier one-round rejected grids remain in
mpi{1,2,4}_meshes/engine_rejected.su2. Prior box grids are byte-unchanged.

New parent fixture profiles include source+mesh migration, worker communicator,
worker execution/wait, return, test audit/export and other. All85 cycle profiles
close; engine/source observations nested. Synthetic metric preparation recorded
once per case. No ParMETIS/CFD/rebuild/transfer costs or full active-rank MPI wait
certificate. Single pilots not scaling evidence; do not compare changed workloads
as fixed work. Bounded-memory models are not RSS/OOM certificates.

Diagnostic fixes: canonical audit donor serialization; explicit-communicator,
single-writer fixture logs after MPI4 interleaved two long v2 ENGINE lines. Final
v3 profiles complete; raw v1/v2 and canonical re-audit provenance preserved.
All90 canonical anisotropic artifacts match, with all54 earlier box artifacts
unchanged. Runner rejects malformed profiles and retains/audits nonzero-command
records; forced-exit7 helper control exports actual saved rejected SU2 mesh.

ANISOTROPIC_MATRIX_PLAN.md and ANISOTROPIC_ENGINE_CHECKPOINT.md give scope/replay.
Next: coarsening/general repair and larger-source controls, then partition/runtime/
CGNS/CFD and closed performance/cluster envelope. These are already thin domains,
not Euler-to-BL construction; production3D remains rejected. 2D/MMG and original
sensor/finer demand/geometric BL/fade/noise policies remain intact.

## Native 3D incremental refinement and frozen scalar reuse — 2026-10-10

Working folder: /media/rausa/4TB/SU2_Versions/SU2_NativeIntegrated.
Branch codex/native-3d-core; predecessor 0d09c431ea5b07099cf7d671c5c5f9169e4a2433.
Goal 1 remains ACTIVE with its entire original scope.

CNativeEngine3D now performs complete incremental frozen-target refinement on
explicit MPI workers, with complete dependency imports, immutable original sensor
queries, planar surface subdivision, private metric progress and atomic directory
publication. Accepted input is untouched; failure discards the private event.
Selection measures initial cells once and only published children afterward.
Owner-preserving routing is not yet weighted ParMETIS; general coarsening/repair,
anisotropic engine campaigns and production runtime integration remain unfinished.

Final engine_controls_v3 PASS MPI1 (54 cases/2891 assertions), MPI2/4 (16 cases/rank),
N=M and M<N plus actual non-MPI rebuild. Nine independently audited completed grids
have 48 tetrahedra/27 nodes/48 physical triangles from the six-tet box; q_min=.756,
J_min=.577, max metric length=1.732, all twelve original markers retained. View
integration_evidence/native_3d_core_v1/engine_controls_v3/mpi{1,2,4}_meshes/engine_full.su2
and engine_m*.su2; one-round rejected grids are retained as engine_rejected.su2.
Accepted input is byte-preserved on round/storage/options/ID-range rejections.
Donor transport/P1/composition residuals remain zero. No CFD state transfer claim.

Measured dominant cost was exact original-source edge tracing, not interpolation.
Same-target, unchanged-endpoint scalar reuse reduces MPI1 field edge integrations
819->234, MPI2 819->306, MPI4 861->358. All 54 JSON/SU2 artifacts and accepted work
match pre-reuse v2 byte-for-byte within each partition. No predicate/target relaxation.
Composed targets cannot use this sensor-only shortcut; unsupported edge integration
still rejects. Three paired MPI1 observations give engine medians .2080->.0763 s
(~2.73 ratio) on identical accepted work; small contended box, not a CFD/scaling claim.
Detail OFF/ON triples have identical work and unresolved overhead within variability.

ENGINE_REFINEMENT_CHECKPOINT.md documents code, evidence, timing scope and limits.
Engine timing excludes final object destruction; external controls include it.
Private CPU and nested query subcosts are separated; total MPI wait/lifecycle,
CFD rebuild/transfer, ParMETIS and CGNS remain unqualified. Conservative modeled
storage is not allocator/RSS certification. V1/v2 logs/pins/snapshots are retained.
Next: anisotropic/spatial engine qualification, repeated coarsening/repair, then
partition/runtime/CGNS and closed performance/cluster envelope. Production 3D
remains rejected; 2D/MMG and finer sensor/geometric BL/fade/noise policies preserved.

## Native 3D distributed immutable donor checkpoint — 2026-10-10

Working folder: /media/rausa/4TB/SU2_Versions/SU2_NativeIntegrated.
Branch: codex/native-3d-core; predecessor b18b5d8ecd91e5975b7a8973679ac80807cad749.
Goal 1 remains ACTIVE with its full scope unchanged.

CNativeDonor3D freezes original tetrahedra and sensor-only nodal tensors per
worker communicator. Hash-routed authority checks reject inconsistent shared
coordinates/sensors, duplicate cell IDs/canonical simplices, invalid SPD and
nonpositive cells before freezing. The house local ADT and at most eight owner
boxes per rank route bounded imports without replicating the original mesh or
scanning every original cell per candidate. Complete conservative AABB unions
above 256 donors reject; exact FrozenField containment/edge coverage remains
mandatory. Empty owners and inactive requesters participate collectively.

Final donor_import_controls_v2 PASS on MPI1/2/4: MPI1 50 cases/2841 assertions;
MPI2/4 13 cases per rank. The distributed split/inverse fixture now validates
with routed immutable donors. Independent Fraction audits verify all 42 actual
transported donor records, 28 canonical P1 samples and manufactured thin-region
composition samples: maximum residual zero. Source/cache-hit controls apply
composition at every actual query. M=N and M<N, genuine empty owners, source
retry/rejection, 257-cell owners and oversized-union rejection pass. All 30
retained grid JSON/SU2 files match the previous checkpoint. Final non-MPI
serial_donors passes; its tiny RSS is 3580 KiB, not a service memory certificate.
Seven focused auditor controls detect tampered/missing/duplicate observations.

DONOR_IMPORT_CHECKPOINT.md records scope, limits and replay. Parent Build/Import
timers are exclusive, routing/search/exchange detail nested; house transport
stats remain partial. No speedup, instrumentation overhead or scaling claim.
The source must already be embedded/conforming. AABB import does not authorize
outside-domain queries or certify candidate coverage. Modeled requested storage
is conservative, not allocator/RSS certification; hard-OOM recovery untested.

Next: incremental distributed candidate engine and repeated full metric
completion, general planar operators, tetrahedral partition weights,
runtime/CFD/history/CGNS integration and performance qualification.
Production 3D remains rejected. 2D/MMG, finer sensor, BL/fade and noise policies
are unchanged. No new papers are required for this checkpoint.

## Native 3D worker-communicator search checkpoint — 2026-10-10

Working folder: /media/rausa/4TB/SU2_Versions/SU2_NativeIntegrated.
Branch: codex/native-3d-core; predecessor f13a08be3eefa2db8e547c10c52569d552f92ad2.
Goal 1 remains ACTIVE with its full scope unchanged.

House CPassiveComm Allgatherv and CRankBoxTree now accept an explicit communicator.
Existing callers retain the default SU2 communicator. This removes the global
collective from rank-box construction when only M of N CFD ranks are workers.
Empty send buffers avoid null-pointer arithmetic; populated buffers keep a
single range copy, without an extra zero-fill. No measured speedup is claimed.

Final worker_field_controls_v2 passes actual MPI1/2/4: MPI1 has 46 cases and
2752 assertions, including all 37 existing 3D kernel cases; MPI2/4 have nine
cases per rank. Subset controls cover M=N, M=N-1 and M=N/2, empty partitions,
127-byte rounds, and both 2D and 3D boxes while inactive ranks wait on the parent.
The final non-MPI serial_transport control also passes. Source and artifact
hashes were verified; all 15 mesh JSON and 15 SU2 files match the published
checkpoint. Independent audit results match too; their source-log hashes
necessarily differ. Earlier worker_field_controls_v1 evidence is retained.

WORKER_COMMUNICATOR_CHECKPOINT.md records scope and replay. This is a shared
routing prerequisite, not a distributed original-sensor service or an enabled
3D remesher. No full SU2/AD build, CFD regression or scaling campaign was run.
Next: immutable distributed sensor donor import/query, incremental MPI engine,
full metric completion, planar operators, tetrahedral partition weights,
runtime/CFD/history/CGNS integration and performance qualification.

## Native 3D distributed dependency/publication checkpoint — 2026-10-10

Working folder /media/rausa/4TB/SU2_Versions/SU2_NativeIntegrated;
branch codex/native-3d-core, predecessor32512ee03632c07c009b543d18f61feabf8b6dbe.
Goal1 ACTIVE/full scope unchanged. New CNativeDistributed3D reuses house passive
transport/election/wire fields on explicit worker communicators. Routed complete
vertex/edge/face stars, authoritative versioned payloads, conflict claims, staged
mesh+directory publication and common rejection. Global cell identity directory
also rejects duplicate IDs with disjoint nodes. Cached resident accounting and
caller/scratch admission avoid per-candidate global scans. Metadata protocol only:
caller must approve complete geometry/embedding/markers/metric and reserve IDs.

Final distributed_controls_v3 PASSMPI1/2/4 (8distributed cases/rank; MPI1 also37
existing3D cases,45total/2749assertions). Actual house MPI/ADT objects rebuilt.
Split+inverse publish across cyclic ownership, rejection preserves bothstores.
N=M1/2/4,2->1->2,4->2/3->4 mesh/version/facet-record migration/return PASS.
15actualMPIframes independently exact embedding/facet/marker/P1 audited, SU2
files in distributed_controls_v3/mpi{1,2,4}_meshes. Private refined/returned maxL
2.828 remainsoversized; coarsened maxL1.732 strictPASS. No complete sweep claim.
v1preserves const-map-key test helper compilationfailure; no MPI run in v1.
Firstpassingv2retained; finalv3adds repeated-build rejection, all30grid/JSON
artifactsunchanged. Rebuild after migration requires a new directory.

DISTRIBUTED_CHECKPOINT.md documents exact scope, limits, pins/replay/phasecaveats.
Directory publicphase totals exclusive; electionnested. World transportpartial
includes fixturecalls and omits internalfail-elections; not totalMPIwait/closure.
Modeled requestedstorage notallocator/RSS certification; overflowprobe SIZE_MAX
is attempted rejectedbound. Single tinycontendedruns notscaling/speedupevidence.

Next distributedoriginalsensorqueries, incrementalMPIengine/repeatedcompletion,
general planaroperators, tetrahedralParMETISweights, runtime/CGNS/CFDhistoryreturn
and full performance/memory/imbalancequalification. Runtime3Dstillrejected;
2D/MMG/finer-sensor/geometriccomposition/fade/noise unchanged. No newpaperneeded.

## Native 3D exact-arithmetic cost checkpoint — 2026-10-10

Working folder /media/rausa/4TB/SU2_Versions/SU2_NativeIntegrated;
branch codex/native-3d-core, predecessor f5e94f46624c100520dcf0da59561059d1f7457a.
Goal1 ACTIVE/full scope unchanged. Product/difference skip only redundant zero
limb ranges; exact bit representation/sign/overflow/guards remain unchanged.
No public API/metric target/2D changes. Product-only performance inconclusive(.3%);
combined arithmetic retained after repeated fixed-work evidence.

arithmetic_controls_v2 PASS37cases/2651assertions+290Fraction signs+6272bitwise
product/difference/overflow controls. arithmetic_edge_oracle_v2 PASS104edges,
71covered33rejected147intervals; numerical error bounds unchanged.
arithmetic_optimized_v2 PASS5complete independently audited mesh cycles;
all15SU2+15JSON byte-identical to prior accepted coupled_meshes_v2 grids.

arithmetic_comparison_v2 PASS12sequential runs,3per binary/detail mode;
all meshes/work/residuals identical,182edits/8390evaluations/1556reuses.
OFF median2.577178->2.427019s (5.83%less,70.62->74.99edits/s), CPU2.57->2.42s.
All3candidateOFF times below3baselineOFF; one-core tiny envelope only.
TimerON wider ranges: overhead explicitly measured, primary claims useOFF.
Phaseclosure/CPU/RSS/loads/binary/source pins retained. ARITHMETIC_CHECKPOINT.md
explains strict scope/replay, inconclusive v1 and compact repeat-frame publication.

Selection/source queries still dominant. Next distributed complete-star dependency
and version/atomic admission using existing passive exchange/failure election;
incremental selection is required in that engine, not toy full rescans. General
planar association/coarsening and real composed integration remain pending.
Runtime/MPI/CGNS/CFD/history transfer/scaling unqualified;2D/MMG/fade/noise unchanged.
No newpaperneeded; AdapNoExt/external scripts/foreign processes untouched.

## Native 3D coupled planar boundary and metric checkpoint — 2026-10-10

Working folder: /media/rausa/4TB/SU2_Versions/SU2_NativeIntegrated.
Branch codex/native-3d-core, predecessor5f46583e5b81b95aa4dba9f538f2ae0a9573bdca.
Goal1 ACTIVE with full original scope. Coupled complete edge-star splits and paired
inverse coarsening now adapt physical triangles with immutable finite facet/marker
authority, exact orientation/skin and manifold links, atomic private output rollback.
Actual-centroid target shape and source-resolved unique-edge gates separate private
progress from final .20/.05/1.8 completion. Repair controls support quality/size
progress for movement/reconnection. Combined-target edge integration rejects until
geometrically resolved, never substitutes sensor length; point composition intact.

Final strict-linked unit validation boundary_metric_controls_v3 PASS37cases /
2651assertions plus290 Fraction signs, with Git absent and listed-source snapshots.
Earlier runner metric recompilation corrected; all earlier evidence retained.
Five full manufactured serial cycles in coupled_meshes_v2 independently PASS:
182 edits, geometry/marker/embedding/P1/metric audit; box/thin1e4/thin1e8/sheared
6->48->6 tets and12->48->12 wall triangles, rotated_spatial6->40->6 and12->40->12.
Accepted grids at integration_evidence/native_3d_core_v1/coupled_meshes_v2/*_adapted.su2.
These are adapted manufactured meshes with no CFD state, not source-only grids.
Coarse event snapshots accepted adapted donors. All15SU2+15JSON unchanged fromv1.

Single contended phase observation totals2.723354s: selection1.247643s/private
metric1.337250s/geometry.064687s/other.073774s.8390edge evaluations/1556reuses.
Source tracing and repeated full toy-mesh scans dominate; prioritize those before
larger runs. Nested query costs are not added to parents. No scaling/speedup claim.
BOUNDARY_METRIC_CHECKPOINT.md gives limits, receipts and sequential/no-Git replay.

Exact on-edge planar proposals only; general facet coarsening/rounded-point
association pending. Thin mapped cases are not Euler-to-BL construction. Local
star authority/IDs/embedding remain preconditions, no distributed completeness.
Runtime still rejects3D; no fullMeson/AD/CFD/MPI/CGNS/transfer/performance envelope.
Next query/selection cost and general planar admissibility, then actual distributed
admission/weighted M<=N/return/transfer and fullGoal1matrix.2D/MMG unchanged.
Local papers remain sufficient; AdapNoExt/external repair project untouched.

## Native 3D original-sensor edge checkpoint — 2026-10-10

On codex/native-3d-core, new bounded TraceSegment uses exact rational original-tet
crossings, ordering/coverage, per-donor endpoint weights and cancellation-free
interval widths. FrozenField::SensorEdgeLength analytically integrates original
P1 SENSOR only. One-donor contained edges bypass rational tracing; multi-donor
edges keep the exact path. Thin source strips between3point samples and intervals
whose crossing parameters both round to0.5 are retained without coarse spreading.
Chord norms have magnitude/exact cancellation checks and reject underflow sentinel
substitution. Existing point composition still runs at every actual query.
Sensor audit is explicitly insufficient for composed BL/target acceptance.

Final PASS28cases/2373assertions+290 Fraction signs in native_3d_core_v1/edge_controls_v4.
Independent edge_oracle_v2 PASS104edges/37groups,71covered33rejected147intervals;
lengtherror4.1162e-17, weighterror4.8867e-20, widtherror7.1193e-20.60direct intervals.
Three source slab SU2 grids+sensor JSON/edge defects independently audited there.
They are manufactured SOURCE grids, not adapted CFD outputs. Point field_oracle_v5
PASS210queries and output byte-identical to v3. Preserved v1 fixture failure;
v2 has explicit source-provenance caveat, final runners reject in-flight edits.

Separate optional exclusive edge search/trace/integral timers, rejection/work counts
and bounds added. Single contended control observes trace/containment dominating;
no speedup/scaling/affordability claim. One low-priority job at a time under foreign
Python/4rank FSI. No full SU2/MPI/CGNS/AD run. EDGE_CHECKPOINT.md gives full scope,
receipts/replay, performance observations and unchanged original completion contract.
Next actual geometric composition integration and metric-gated private proposals,
then adaptable planar surface and full MPI admission/partition/transfer. Runtime
still rejects3D;2D/MMG/fade/noise unchanged. Goal1 ACTIVE; no additional paper needed.

## Native 3D immutable sensor/query checkpoint — 2026-10-10

Prior goal turn progressed with cavitye7f87ecf4a published. New strict Barycentric
uses exact signs and1e-13 magnitude admission, falling back to bounded exact
integer arithmetic for thin rotated ratio accuracy. Exact tensor admission shared
with Measure. New FrozenField snapshots sensor-only original tetrahedra, sorts
canonical node keys, validates shared values, reuses SU2 ADT for multiple donors
and bypasses an unnecessary singleton tree. Bound256donors/2048FIFO sensor samples.
Every query recomposes at the actual point including cache hits; composed BL
nodal tensors are never interpolated/cached. Actual-query semantics and finer
sensor demand tested with a manufactured thin floor, not a real3D BL provider.
Exact containment only: associated physical roundoff/extension remains pending.

PASS22cases/2294assertions+290 exact signs in announced
SU2_NativeIntegrated/integration_evidence/native_3d_core_v1/field_controls_v4.
Actual house ADT/base and serial MPI-wrapper compiled/linked, no stubs/full solver.
PASS210 independent Fraction indexed queries in field_oracle_v3,150inside60outside,
aspect1..1e10/scales1e-50..1e50/translations0/1000. Maxweight8.1262e-17,
directional tensor defect1.5378e-16 tolerance1e-12; fine nodalzz1e10..4e10.
Oracle v1/v2/v3 result lines byte-identical. Initial Werror unused-header-parameter
failure retained; runner matches project Wno-unused-parameter. Earlier field
source/test snapshots and failed runner match receipt hashes for replay.

FieldStats records queries/hits/misses/failures, actual interpolation/composition
attempts including rejection, candidates/containment/cache bounds/index bytes and
optional exclusive query child timers under query parent.256 separated donors
produce1candidate/1test. Build/caller-copy scopes documented; memory ceiling and
full MPI/event closure/instrumentation overhead/scaling still pending. One owned
low-priority process at a time under foreign Python/MPI; no full Meson/CFD/MPI/AD
run or speedup claim. FIELD_CHECKPOINT.md is the current detailed scope/replay.

Next actual metric/shape/edge acceptance with source/geometric integration
breakpoints, physical-boundary roundoff/extension and coupled planar surface
operators, then MPI admission/weighted partitions/return/transfer/full validation.
Native runtime support still rejects3D.2D/MMG source/fade/noise untouched.
Goal1 ACTIVE and full original scope unchanged.


## Native 3D private cavity reconstruction — 2026-10-10

Previous goal turn made progress: incidence4be53587cc published. New bounded
private Cone/ValidateFixedInterface support insertion/movement/removal and some
reconnections with unchanged oriented interfaces, exact positive cells, full
sphere/disk links and explicit64/128 source/replacement budgets. Rejection leaves
output unchanged and reports obstructing face/apex IDs. This is geometric private
closure on an embedded accepted source, not metric/embedding/MPI acceptance.
Runtime support still rejects3D; physical planar surfaces remain adaptable in Goal1.

Strong regression reproduced false rounded-volume closure rejection at rotated
1e6/1e8/1e10 aspect (cavity_controls_v2). Preserve failed logs and old source/tests.
Replaced redundant rounded determinant sum with exact oriented-face/coordinate
closure, retaining positivity/topology guards. PASS16cases/163assertions plus290
Fraction signs in cavity_controls_v3. Initial15cases155assertions also retained.

Inspectable initial/private-candidate SU2 meshes+JSON in
SU2_NativeIntegrated/integration_evidence/native_3d_core_v1/cavity_meshes_v2.
Independent Fraction geometry/volume/skin and pairwise tetrahedron SAT PASS8probes.
Five actual constant-tensor controls enforce q>=.20, J>=.05, L<=1.8; qmin.348566,
Jmin.156174, Lmax1.414214. Three very thin geometry-only controls explicitly fail
isotropic shape gates; not accepted metric meshes. All10 earlier v1 SU2 files
byte-identical. CAVITY_CHECKPOINT.md explains scopes/replay/legacy source pins.
One owned small process at a time, nice19/single library thread, under foreign
Python/MPI work; no full build/CFD/MPI. Timings observations only, no scaling claim.

Next immutable sensor-only tetrahedral donor indexing/query composition and metric
acceptance; then richer operators/targeted growth/physical surfaces and distributed
admission/weighted M<=N partitions/return. All original correctness/performance
requirements remain. Goal1 ACTIVE; AdapNoExt/external repair repo untouched.


## Native 3D local incidence and reference check — 2026-10-10

Geometry checkpoint191f35fd58c0cd816b1e1f786cdbbf5441a3099b published and remote
verified on codex/native-3d-core. User asks independent robust/efficient design,
using repair scripts and existing papers as evidence rather than prescriptions.
Local Loseille2017 cavity, Tsolakis2021 parallel and Galbraith2020 verification
sections reviewed; reading scope/paths/hashes in native_3d_core_v1/papers_reference.json,
design decisions/limits in DESIGN_REFERENCES.md. No new paper requested yet.

New Node/Cell records and immutable sorted face/edge/vertex index reject invalid
identity/coordinates/volume/face cancellation and provide oriented boundary and
face-component diagnostics. Queries cover supplied cells only: no MPI-complete
star, vertex-link or geometric embedding certificate. Build O(k log k), memory
O(k), query O(log k + star size); private bounded-patch use intended. No cavity
replacement or full driver support yet; native2D implementation untouched.

PASS10Catch cases/93assertions plus290 exact Fraction signs in announced folder
SU2_NativeIntegrated/integration_evidence/native_3d_core_v1/topology_controls_v1.
One sequential nice19 single-core check under host load~3.8 and one foreign
Python job; no fullSU2build/CFD/MPI. Sources/tests wired into Meson; full link
pending. No adapted grid produced. Goal1 ACTIVE, remaining full scope unchanged.


