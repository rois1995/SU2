## Native 3D production runtime checkpoint — 2026-10-11

Repository `/media/rausa/4TB/SU2_Versions/SU2_NativeIntegrated`; branch
`codex/native-3d-core`; predecessor `5d83dcda7958f5af81ddeb3235b1a0a56f686a01`.
Published runtime implementation commit `28bb42edc7e191b07699ea6eac9090968aa93eb5`,
pushed to origin and full remote SHA verified. Checkpoint report:
`native_3d_core_v1/RUNTIME_REPORT.md` and
`RUNTIME_CHECKPOINT_VALIDATION.json`. Actual factory/ReplaceMesh/accepted-reference
notification, durable original faceted sidecars, M1..N on MPI1/2/4: 84 accepted
driver events,42 exact OFF/ON controls,42 SU2/CGNS output/readbacks, sensor error0,
maximum affine current/BDF2 error1.119683e-12; qmin.628068,Jmin.397369,Lmax1.793742.
Separate actual native2D driver and MPI-aware 2D/3D MMG regression PASS MPI1/2/4.
Default no-native-repartition path and rejection after private accepted edits,
retry/state preservation/output audits also PASS MPI1/2/4. No owned build/test
remains live: runtime regression session68072 and defaults24623 returned PASS.

Historical `runtime_v3/validation.json` remains FAIL due to wrong serial-only MMG
unit selection on MPI2 AFTER all 3D stages/audits passed. Follow-up correct
MPI-aware regressions PASS with the same production sources/archives. Manifest
links both; no failure rewritten. Earlier wrong build target/runner setup and
lexical-marker-order encoder failures retained. No MMG implementation changed.

Inspectable final meshes/states (manufactured cube, not real CFD):
`integration_evidence/native_3d_core_v1/runtime_v3/mpi4/`
`runtime_barycentric_m4_d0_mesh_00003.su2` and adjacent original `.native_ref`,
`runtime_barycentric_m4_d0_s2_written_CGNS.cgns`,
`runtime_barycentric_m4_d0_s2_states.json` and mesh `.json`.

Full Goal1 ACTIVE. Next priorities: complete metric/index/query/repartition/
return/rebuild/transfer per-rank exclusive lifecycle reporting and CPU/wait/RSS,
aggregate live-buffer admission (initial import and caller-held copies currently
not fully covered), actual driver reload/restart, larger/thinner/rotated lifecycle
cases, repeated overhead and practical size/rank qualification, then closed SGE
scripts. Current phase closure is remesher-only; no scaling/affordability claim.
3D requires planar adapted boundaries/no BL/noise0; original reference limit256
triangles remains. Future combined reusable runner assembled from qualified
components has syntax checks but was not redundantly relaunched. No cluster-launch
requests tonight; foreign FSI, AdapNoExt and ClusterResults left untouched.

## Native 3D actual geometry/solution bridge validated — 2026-10-11

Latest checkpoint preparation: geometry_bridge_v10 and geometry_meson_v1 both
terminal PASS, all source/archive/thin-member pins unchanged. No owned build/test
is running after session68185 returned terminal PASS. New CNativeImport3D and
finite oriented ContainsFace are validated against actual CFD geometry, weighted
M<=N core, returned readers, geometry/solver reconstruction, current/BDF2 transfer,
SU2/CGNS writers/readers and independent audits on MPI1/2/4, M1..N. Each matrix:
42 events,84 audited frames,42 state pairs,42 raw output roundtrips; minq.367927,
minJ.146382,maxL1.793742,transported sensor residual0,maxstate8.471971e-13.
Shared 2D fixture regression passes. See native_3d_core_v1/GEOMETRY_BRIDGE_REPORT.md.

At published 5d83dcda, production native3D factory remained guarded. Full Goal1 ACTIVE: wire the opt-in
runtime and durable planar authority, then close complete lifecycle phases/CPU/
wait/RSS/memory and OFF/ON/repeated practical envelope before claiming completion.
Current one-shot timings include tests/output and changed trajectories; no scaling
or affordability claim. Existing underfilled CFD ranks remain a documented limit.
No cluster requests tonight. Leave AdapNoExt/foreign submodules/ClusterResults alone.

### Earlier bridge setup and fixture diagnosis (retained)


Repository /media/rausa/4TB/SU2_Versions/SU2_NativeIntegrated; branch
codex/native-3d-core; predecessor e19af2b2f22654d6ba95862761663a385d4b15ca.
Goal1 ACTIVE/full scope. CNativeImport3D imports actual CFD-owned
sensor-only tetrahedra, halo coordinates and oriented physical incidence. It
retains bounded immutable faceted authority, rebinds finite triangles after reader
renumbering, queries original P1 in bounded batches for weights and returned reader
metrics, and preserves output on elected failures. No production driver factory
has been wired; native runtime3D guard remains.

Geometry bridge v1..v5 failed during standalone build setup/linking, BEFORE TESTS:
v1 METIS include; v2 public matrix include; v3 MEL include; v4 reduced standalone
house-helper duplicates; v5 actual SU2 archives retain pre-communicator ABI. Do not
claim adaptation/state/output validation from these receipts. All failures remain.

Actual 268-step current-ABI Common/strict/CFD-core rebuild geometry_build_v1
is terminal PASS, exit0, no production source changes during the build. The
follow-up geometry_followup_v1 is terminal FAIL: it launched v6, whose test helper
incorrectly encoded a vector as one record. Fixed by encoding individual records.
V7 compiled/linked but aborted in CConfig before adaptation: COMPUTE_METRIC=YES
requires ADAP_SENSOR. geometry_bridge_abort_v1 captures the existing v7 binary's
one-rank debugger stack proving this cause. The fixture now supplies PRESSURE.
No production config guard or numerical contract changed.

Validated auditor checkpoint pushed/remote-verified:
a4806f8e796d564d6afae0e36b593d8aae7c23b8. Actual bridge is now validated for checkpoint publication.
V8 reached adaptation/reader return but stopped at the existing end-step history
contract: current solution must equal time_n after the dual-time update.
geometry_bridge_abort_v2 proves that cause. Fixtures/checker now use identical
current/time_n and distinct time_n1; no transfer contract changed. New lightweight
geometry_audit_v4 PASS includes six adversaries, adding mismatched time_n.
Historical v3 fixture used a synthetic independently scaled time_n; its old
checker source pins remain, but it is not an admissible production transfer input.

V9 terminal FAIL: MPI1/2 tests and independent audits PASS (12/24 frames,6/12
state pairs and6/12 real SU2/CGNS roundtrips). MPI4 completed all 3D worker choices
M1..4 and both methods, then the added six-point 2D MG fixture crashed in
CPoint::SetPoints/BuildGeometryFVM with an empty sparse graph on two ranks.
No 3D adaptation/transfer numerical failure was observed. The supplemental
four-rank independent audit is terminal PASS (48 frames,24
state pairs,24 actual roundtrips); the whole v9 receipt remains FAIL.
The shared 2D fixture now uses the existing transfer tests' n4->n6 meshes, avoiding
underfilled CFD partitions without changing production geometry handling.
The corrected geometry_bridge_v10 matrix subsequently passed in full.
Original complete v9 receipt stays FAIL, including the 2D crash.

Test-only bridge includes bounded actual owner/halo geometry and sensor import,
retained planar marker authority, weighted explicit M<=N worker communicators,
returned distributed original-P1 metric readers, actual geometry/solver rebuild,
barycentric/conservative current/BDF2 transfer, SU2/CGNS write/readback, atomic
reader-budget vetoes, and a manufactured cached/remote thin query-composition
control. That control is not composed3D edge integration or BL generation.
No configured driver/CFD iteration, full phase/CPU/wait/RSS closure or performance
qualification yet. Actual Meson bridge test compilation and archive-linked checks now PASS.

geometry_audit_v3 PASS: lightweight independent-auditor self-check, valid serialized
box/state/SU2/HDF5 fixtures and five rejected corruptions. It is not actual native
output or CFD integration proof. v1/v2 self-check failures retained. Report
geometry_audit_v3/AUDIT_SELF_CHECK.md explains exact scope and pending work.
Auditor checkpoint is already published; both actual bridge campaigns now pass. No cluster questions tonight. Full configured
native3D, lifecycle/CGNS/rejection, performance/RSS/scaling goal is not complete.

## Native 3D weighted partition checkpoint — 2026-10-11

Repository /media/rausa/4TB/SU2_Versions/SU2_NativeIntegrated; branch
codex/native-3d-core; predecessor32dfde2bd01f81a9c90bbd1452bbdfbd307a7d02.
Goal1 ACTIVE/full scope. New tetrahedral triangular-face dual graph uses sparse
64-bit IDs with separate contiguous graph ordinals, two work/storage constraints,
empty-rank graph exclusion, explicit parent/subset communicators and staged atomic
migration. Tetrahedral work uses sensor-only centroid volume plus actual composed
centroid/vertex operation demands; no wall tensor is spread over coarse volumes.
Weights1..1024/batches<=128 are an uncalibrated static proxy. Original CFD/sensor
stores are separate; accepted CFD solution/history transfer is not established.

partition_controls_v2 PASS: three sequential MPI1/2/4 repeats,63 partition cases,
21 weighted M=N/M<N adaptation-and-return events/42 independent frames; actual
nonMPI3 cases/1236 assertions. Exact cube tiling/graph/wire/cut/load/strictP1/
geometry/embedding/markers pass, four audit adversaries plus atomic bad-input/
storage vetoes. Within each rank choice ownership/mesh/source hashes are identical
across repeats. 6->48 cells,27 points,q.755953/J.577350/L1.732051/volume1.
Named grids and original frozen sensors:
integration_evidence/native_3d_core_v1/partition_controls_v2/mpi{1,2,4}_repeat1_meshes/.
Initial v1 PASS preserved; V2 fixes memory-control array and helper-source pins.

partition_meson_v1 PASS actual SU2 Meson strict library and actual partition/2D
geometry test objects, MPI1/2/4 independent audits; MPI1:25 cases9820 assertions.
Strict arithmetic appended after fastmath; MPI/ParMETIS dependency wiring verified.
PARTITION_CHECKPOINT.md and repeat_summary.json retain scope/phase samples.
N1/M1 one-owner partition parent median .005042s; N4/M4 .034161s with large spread;
foreign4-rank CFD was active. This is contended diagnostic evidence, no speedup/
scaling/CPU calibration claim. Requested bound~1.09MB excludes allocator/RSS and
internal(Par)METIS workspace. Private CPU/wait/full lifecycle costs unfinished.

Next: actual opt-in native3D geometry import/reader-slice return/CFD state/history
rebuild and SU2/CGNS output qualification, using existing driver transfer/lifecycle.
Production3D guard remains until connected and validated; same immutable reference
on workers,256facet/donor caps/composed3D integration/nonconvex reference limits
and preceding unisolated6-7% regression remain. No numerical 2D/MMG algorithm or
target changed; only relevant 2D geometry build checks repeated here, not full CFD.
No owned process remains live after checkpoint. Continue nice19/thread1/singlejob,
real host checks and announced full case folders. No cluster launch questions.
Publish validated checkpoints, verify full remoteSHA, preserve historical failures,
leave AdapNoExt/foreign jobs/submodules/ClusterResults alone. Goal1 is incomplete.

## Native 3D finite coplanar wall checkpoint — 2026-10-11

Repository /media/rausa/4TB/SU2_Versions/SU2_NativeIntegrated; branch
codex/native-3d-core; predecessor9557213b3bbc8b1036464bc98a5030a84d449261.
Goal1 ACTIVE/full scope. Exact certified connected convex coplanar same-marker
finite disks remove artificial original facet seams. Canonical minimum facet IDs
allow wall movement/general collapse/inverse sibling pairing across those seams.
Concave/holed/uncertified components retain raw triangles; creases/markers stay.
Core source/target/BL/fade/noise/2D/MMG policies and strict q/J/L gates unchanged.

coplanar_regression_v2 PASS MPI1/2/4 (MPI1:73 cases4652 assertions;23 cases/rank
MPI2/4),1350 prior mesh/source artifacts identical. New21 independent frames/
18 events with N=M/M<N: pyramid4->2->28/30->2; finite area/marker/embedding/
original-P1 audits and4 adversaries. Grids and donor states:
integration_evidence/native_3d_core_v1/coplanar_regression_v2/mpi{1,2,4}_coplanar/.
actual_nonmpi PASS9 passive cases115 assertions and repeated cycles/repair/
rejection/composed-edge refusal; RSS5288KiB only for this small control.
coplanar_sources_v1 PASS63 frames/36 successful events/six exhausted rejections;
126 prior source750 mesh/state artifacts identical. Failed v1 adversary retained;
it accidentally reused its existing facet binding; actual meshes had passed.

Three sequential MPI1/2/4 pilots PASS with fixed mesh/edit/query work within each
rank choice and closed migration/worker/return phases. MPI1 parent medians:
coarsen4->2 .007931s; refine2->28 .023182s; coarsen28->2 .036094s. Geometry build
8facets .000136s; requested engine work~2.23MB is not RSS. More workers worsen
this tiny case and may change mesh trajectory. Three alternating actual nonMPI
checkpoint pairs match work but candidate is6-7% slower. Shared tracing/field/
metric objects identical, final addresses shifted; cause not isolated, no layout
flags. COPLANAR_GEOMETRY_CHECKPOINT.md retains all receipts and limits.
Streamed reconstruction CPU scope now excludes failure election; full private
CPU/wait qualification remains pending.

Next: tetrahedral cost-weighted partition/actual runtime/CFD/CGNS integration;
general finite-union authority and measured regression remain gaps. Production3D
guard,256-facet/256-donor budgets and composed3D edge-integrator refusal persist.
Same complete immutable geometry authority required on each worker. Do not
complete Goal1 from frozen pilots or claim BL/unsteady3D/CFD affordability.
Continue one owned job,nice19/thread1/MPI<=4; real host checks, announce full case
folders, leave foreign processes/submodules/ClusterResults and AdapNoExt alone.
No cluster launches requested tonight. Push every validated checkpoint and
verify remote full SHA; keep all failures and compact verified evidence aliases.

## Native 3D private-query checkpoint — 2026-10-11

Repository /media/rausa/4TB/SU2_Versions/SU2_NativeIntegrated; branch
codex/native-3d-core. Predecessor dfe841a4ceaa0e7d453960e9afa386e28d7271f4.
Goal1 ACTIVE/full scope. Cap-only known-query private batches now complete the
previously rejected source750 wedge, general wall coarsening and interior repair.
Each proposal gets its own centroid/edge support. Geometry-first skips imports
for impossible already-admissible coarsening; all frozen gates/source/policies
remain. Full source/admission failures reject atomically; bounded query cap stays.

private_query_regression_v4 PASS MPI1/2/4,70 cases/4565 assertions MPI1,22
cases/rank MPI2/4;1350 prior mesh/source artifacts identical. actual_nonmpi
PASS9 cases/115 assertions and repeated cycles/repair/rejection. sources_v3 PASS
MPI1/2/4/M<N:63 frames/36 successful events/six exhausted private rejections;
126 before/after optimization artifacts identical; exact independent audits
and seven adversaries pass. Inspectable grids/frozen metric states in
integration_evidence/native_3d_core_v1/private_query_sources_v3/mpi{1,2,4}_meshes/.
Initial/accepted/rejected SU2 and .json included; no CFD solutions in these pilots.

Three alternating pinned source pairs preserve target/mesh/edit work. Coarsening
parent median .443577->.136109s (~3.26x) with13 rounds/one accepted/12 rejected;
batches19->7, private retries13->1. Other default MPI1 cases6-9% slower; CPU6
pairs reproduce this. A diagnostic-only relink restoring unchanged kernel
addresses restores overlapping unchanged-case timings. All unbound/pinned/
layout receipts retained, no production link flags added. Non-MPI OFF/ON pairs
against prior checkpoint overlap. PRIVATE_QUERY_CHECKPOINT.md gives full figures.

v1 unit fixture omitted facets; v2 wrongly used fixed-skin coning at a new
boundary point; v1 source audit applied the
accepted shape gate to deliberately distorted initial input. Failures retained;
accepted gates remain strict. No owned process remains live after publication.
Next: finite coplanar geometry authority, then actual runtime/weighted partition/
CFD/CGNS and full performance qualification. Source2058 needs294 donors and
still rejects; source1296 measurement is not an adaptation result. Streamed
proposal CPU scopes include election CPU; narrow before private CPU/wait claims.
Production3D guard, original seams and composed edge-integration refusal remain.
Do not complete from frozen fixtures or claim BL/unsteady3D/CFD affordability.
No cluster launches requested; user unavailable overnight. Keep one owned job,
nice19/thread1/MPI<=4, real host checks, announced full folders, and push every
validated checkpoint with a verified remote SHA.

## Native 3D closed-query filter checkpoint — 2026-10-11

Branch codex/native-3d-core; predecessor fcda83cb2b3378c55124c474e25da3aeb33f3d20.
Goal1 ACTIVE/full scope. Exact-sign Orientation settles plane separation and
endpoint containment before rational clipping; unresolved outside-endpoint
crossings retain exact roots. No target/cap/trace/BL/fade/noise/2D/MMG changes.
query_filter_regression_v1 PASS MPI1/2/4 and all independent audits (MPI1:69
cases/4542 assertions;22 cases/rank MPI2/4). All1350 preceding mesh/source files
match. query_filter_sources_v1 PASS MPI1/2/4/M<N: all72 larger-source grid/source
artifacts match prior;36 frames/18 successful events/six private cap failures;
box/thin/shear P1/closed coverage and six adversaries pass. Non-MPI build not
repeated for this MPI-independent predicate; preceding evidence stays retained.

Three alternating pinned MPI1 source binary pairs PASS with identical query,
donor/edit and mesh work. Source750 valid measurement median .617780 -> .210759s
(ranges .617252-.620717/.208175-.212063), about2.93x in this no-edit fixture.
Six-split wedge median .121119 -> .113765s; rejection .110705 -> .042506s is not
successful adaptation. QUERY_FILTER_CHECKPOINT.md and receipts/manifest give
full scope and retained hashes. No owned campaign remains live. No cluster
launch request; nice19/thread1/single job policy and real host logs retained.

Next: staged known private queries/bounded streaming to remove the whole-cavity
cap, finite coplanar geometry patches, then weighted partition/runtime/CFD/CGNS/
full lifecycle/cluster qualification. Original seams and production3D guard
persist. Do not complete Goal1 from frozen manufactured cases or claim general
scaling/BL/unsteady3D/CFD affordability from this measured local improvement.

## Native 3D query-support checkpoint — 2026-10-11

Repository /media/rausa/4TB/SU2_Versions/SU2_NativeIntegrated; branch
codex/native-3d-core, predecessor a1a23b12c4c9ccd174c8147a0d5620a9c9ade32f.
Goal1 ACTIVE/full scope. Exact closed centroid/six-edge donor imports admit
coarse measurements that exceeded whole-cell volume imports. Shared Centroid,
canonical original sensor-only P1 and actual composition/cache-hit policy are
preserved. Donor cap256/q>=.20/J>=.05/L<=1.8 stay fixed. Whole-cell imports remain
the fast path; only a one-cell cap failure gets one query-support retry. Full
private reconstruction still uses whole-cell coverage; fixed wire records remain.

query_support_regression_v3 PASS MPI1/2/4 and independent audits;68 cases/4535
assertions on MPI1 and22 cases/rank on MPI2/4. All1350 previous mesh/source artifacts
are byte-identical. Actual non-MPI9 cases/115 assertions plus cycles/repair/
recovery/exhaustion/rejection/composed-edge-refusal PASS. Final three paired
baseline/new process runs have matching work and coarsen ON medians .194757913/
.194757800s with overlapping ranges; no general speedup/scaling guarantee.

query_support_sources_v3 PASS MPI1/2/4/M<N. Sources162/384/750/1296 use162/150/
188/242 donors per coarse tetrahedron;384-source thin8/shear4 also pass. Source2058
needs294 and rejects at256. 18 successful events include12 valid coarse no-ops
and six wedge1->8/six-split refinements; six source750 private-cavity cap failures
preserve input. All36 initial/accepted/rejected frames pass independent audits;
9828 closed SAT pairs,60480 exact query-support comparisons,252 canonical/P1/
composition observations and648 analytic edge observations; six oracle adversaries.
All72 larger-source mesh/source artifacts match earlier policies. Inspect
query_support_sources_v3/mpi{1,2,4}_meshes/*initial.su2, wedge_m*.su2, dense_m*.su2,
dense_wedge_m*_rejected.su2 and matching original/source JSON.

Two rejected performance policies are retained: always-query imports (~.42s wedge
vs~.13s prior) and delayed zero-width trace rejection (.2371s coarsen vs.1951s).
Final positive-width tracing restores original early rejection while donor import
keeps closed contact. QUERY_SUPPORT_CHECKPOINT.md, receipts, identities, summaries
and query_support_checkpoint_artifacts.json give replay/provenance/deduplication.
No binaries/objects/cache or duplicate historical grids are published. One owned
heavy job, nice19/thread1/MPI<=4; actual host contention recorded. No jobs remain
from this campaign. No cluster launch requests overnight.

Next: bound and stage actual private queries where full-cavity imports exceed256,
then finite coplanar geometry-patch authority and weighted partition/runtime/CFD/
CGNS/full lifecycle and cluster qualification. Original planar seams and production
3D guard persist; reported work bounds are not RSS. Source2058 query-cap failure
also needs smaller query batching/streaming. Goal1 is not complete; do not treat
manufactured frozen probes as production/BL/unsteady3D/affordability validation.

## Native 3D larger-source checkpoint — 2026-10-11

Working folder: /media/rausa/4TB/SU2_Versions/SU2_NativeIntegrated.
Branch codex/native-3d-core; predecessor7bf0eaa1c2b49edeeb3ea2d17daec1c513f743d4.
Goal1 ACTIVE/full scope. Strict cell-shaped imports exclude exactly separated
face planes only when the global original source can exceed256 donors. Smaller
sources retain cheaper boxes. Original sensor-only P1/canonical source coverage,
actual-point composition, strict q/J/L, atomic caller-input preservation and all
2D/MMG/BL/fade/noise paths remain unchanged. Fixed-size wire records remain mandatory.

large_sources_v4 PASS MPI1/2/4 and M<N: source384 imports170 per coarse tetrahedron;
12 successful events include six wedge1->8 refinements/six splits each. Source750
fails six events at the unchanged donor cap, despite admissible input meshes.
27 initial/accepted/rejected grids pass exact independent geometry/marker/P1
metrics;9828 closed SAT pairs/144 P1-composition observations/three oracle
adversaries pass. Inspect large_sources_v4/mpi{1,2,4}_meshes/*initial.su2,
wedge_m*.su2 and oversized_m*_rejected.su2 plus corresponding original/source JSON.

Full regression_v3 MPI1:66 cases4508 assertions; MPI2/4:21 cases/rank. Actual
non-MPI cycles/repair/recovery/rejection and matched timer controls PASS. All1350
prior mesh/source JSON/SU2 artifacts are byte-identical; mesh_identity.json proves it.
Unconditional filtering was correct but cost~.36s small coarsening; v1/v2 evidence
preserved, policy rejected. v3 fixed-record assumption failure preserved. Final
three alternating old/new binary pairs have matched work and coarsen ON medians
.1941285/.1951316s with overlapping ranges; no general performance claim.

Sources384 wedge event maxima .10-.13s include migration/worker comm/core/return,
with measured tracing/filtering costs. Frozen source preparation and diagnostic
export/audits are separate; actual partition/CFD/rebuild/history/CGNS absent.
Reported modeled wedge work bound<=3,149,784bytes is not RSS. Host contention
recorded; one owned heavy job at a time, nice19/MPI<=4/library threads1.
LARGE_SOURCE_CHECKPOINT.md and final receipts/summary give exact scope/replay.

Next: transformed larger-source/multiple/partial cavities, measured query-footprint
imports to remove the coarse/source750 cap limit, and coplanar finite geometry
patch association; then weighted partition/runtime/CFD/CGNS/full lifecycle and
cluster qualification. Original coplanar seams and production3D guard persist.
Do not complete Goal1 from these frozen manufactured probes.

## In-event native 3D repair recovery checkpoint — 2026-10-11

Working folder: /media/rausa/4TB/SU2_Versions/SU2_NativeIntegrated.
Branch codex/native-3d-core; predecessor2bd703c513577fb0f9a7ea6d8e8c8b73c6d61303.
Goal1 ACTIVE/full scope. Refine rejects can try reconnection; invalid-field
Coarsen rejects can try movement/reconnection. One shared-index/frozen-source
repair queue handles invalid-field stagnation. Normal edge queues stay current;
coarsening resumes afterward. Primary/recovery edits share the original round
budget and atomic path. Target/q>=.20/J>=.05/finalL<=1.8 remain strict.

Exact facet preflight avoids futile movement reconstruction; private bad shape
rejects before edge tracing while actual centroid composition remains mandatory.
Scalar dirty-state reduction reuses one collective. Valid-field coarsening skips
quality-only fallbacks; V1 correctness-passing broad policy rejected for cost.
V1 pilot/source snapshots/logs/audits and V2 work comparison retained.

recovery_integration_v2 PASS MPI1 (65 cases/4493 assertions), MPI2/4 (20 cases/rank)
and actual non-MPI recovery/exhaustion plus existing operator/cycle controls.
216 successful events/324 independent meshes/612 closed parent profiles;
18 needed private fallback acceptances. All918 earlier JSON/SU2 artifacts exact.
Nine adversaries reject. Worst finalq.372093/J.095473/L1.483240. At most35 rounds,
2,166,904 modeled bytes for these fixtures. Regular serial coarsening again has
zero fallbacks and original accepted counts. No general scaling/RSS claim.

Inspect integration_evidence/native_3d_core_v1/recovery_integration_v2/
mpi{1,2,4}_automatic_repairs/restore{R,C}_*_initial.su2 and *_m*.su2/.json.
RECOVERY_INTEGRATION_CHECKPOINT.md/recovery_summary.json describe evidence/limits.
Matched non-MPI instrumentation triples preserve grid/work; source pins stable.
MPI70490/non-MPI10749 terminalPASS; no owned job live. Foreign FSI/interpolation/
mesh-motion jobs were left untouched; local timings are contended.

Next: multiple/partial cavities/larger sources, measured costs, coplanar geometry
patches; then weighted partition/runtime/CFD/CGNS/full lifecycle and cluster
qualification. Finite original coplanar seams remain protected. No new per-private
candidate donor observer; shared field transport/query controls rerun. Manufactured
source preparation is outside subset timing. Actual ParMETIS, CFD rebuild/transfer/
history, full wait/RSS remain unfinished. Production3D guard/2D/MMG/BL/fade/noise
remain unchanged. Do not call Goal1 complete from these manufactured tests.

## Automatic native 3D repair checkpoint — 2026-10-11

Working folder: /media/rausa/4TB/SU2_Versions/SU2_NativeIntegrated.
Branch codex/native-3d-core; predecessor47071504dd6212828176a8e3f6627aa1d5e87a06.
Goal1 remains ACTIVE/full scope. Repair automatically selects complete failing
vertex/edge/face dependencies and bounded movement/reconnection proposals through
the shared frozen-target MPI transaction engine. Accepted input is immutable;
q>=.20/J>=.05/private progress/finalL<=1.8 remain strict. Repair-only storage is
separate and incremental; ordinary cell state avoids an added vector/whole scan.

automatic_repairs_v2 PASS MPI1 (64 cases/3969 assertions), MPI2/4 (20 cases/rank)
and actual non-MPI automatic reconnection/private movement plus existing
6->48->6->48 controls.72 automatic events (36 moves/36 reconnections),108 meshes
including36 initial,204 closed parent profiles. Independent exact embedding/
volume/facet/marker/original-P1 final audits pass; nine tampering controls reject.
All702 prior JSON/SU2 artifacts unchanged. Source pins stable. V1 native MPI1
passed; failed audit runner/parser evidence and successful re-audits retained.

Inspect integration_evidence/native_3d_core_v1/automatic_repairs_v2/
mpi{1,2,4}_automatic_repairs/auto_*_initial.su2 and auto_*_m*.su2/.json.
AUTOMATIC_REPAIR_CHECKPOINT.md/automatic_repair_summary.json document evidence,
performance and replay. Tiny events used at most19 rounds/2,134,136 modeled bytes;
rejected geometry proposals dominate several cases. Three matched non-MPI
instrumentation pairs per event pass (Repair OFF.014612s/ON.014918s medians),
with identical grids/accepted/query work. No scaling/CFD affordability conclusion.

MPI handle13448 and non-MPI49567 terminalPASS; no owned job live.
Next: repair fallback in blocked refine/coarsen, multiple/partial cavities/larger
sources, measured proposal/tracing cost, coplanar geometry authority; then
weighted partition/runtime/CFD/CGNS/full lifecycle and cluster qualification.
Finite coplanar seams remain protected. Automatic mode is separate from
Refine/Coarsen fallback. Private transport/query controls rerun in the shared core,
but this is not a new per-candidate donor-observation certificate. Manufactured
metric preparation lies outside repeated subset-event profiles; actual ParMETIS,
CFD rebuild/transfer/history, RSS and full wait qualification remain. Production
3D guard/2D/MMG/finer sensor/geometric BL/fade/noise remain unchanged.

