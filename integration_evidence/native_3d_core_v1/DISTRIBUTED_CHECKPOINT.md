# Native 3D distributed dependency and publication checkpoint

2026-10-10. Working repository: /media/rausa/4TB/SU2_Versions/SU2_NativeIntegrated.
Branch codex/native-3d-core; predecessor 32512ee03632c07c009b543d18f61feabf8b6dbe.
Goal 1 remains ACTIVE with the complete NATIVE_3D_DEVELOPMENT_GOALS.md objective.

## Implementation

CNativeDistributed3D adds fixed passive OwnedCell records with cell identity,
version and opposite-vertex physical facet bindings. Immutable facet authority
owns markers/geometry; working-cell records carry no composed wall tensors.
It reuses the existing 2D World/RecordStream and actual SU2 passive transport and
failure election unchanged. Matching collectives operate on the supplied worker
communicator rather than accidentally entering the full CFD communicator.

A hashed directory routes four vertex-star records and one global cell-identity
record per tetrahedron. Vertex metadata includes the full sorted simplex IDs;
queries for vertices, edges and faces therefore discover complete authoritative
stars. Admission rejects missing features, stale ownership, duplicate identities,
partial payloads, more than 64 dependencies, unions exceeding that bound, or
vertex-server scans exceeding 4096 entries. Each query list is bounded to 256
features. A 65-cell star is rejected without returning a partial usable patch.
This conservative limit is a current declared rejection boundary, not evidence
that all realistic meshes fit it.

Vertex claims select nonconflicting proposals. Publication first prepares all
replacement mesh nodes and changed directory stars, checks authoritative removals,
coordinate consistency and reused-ID version advance, then elects a common vote.
Repeated initial construction rejects without changing existing metadata; ownership migration requires a fresh directory.
Both accepted stores remain unchanged on rejection; publication performs no new
allocation after that vote. Resident byte estimation is cached and updated only
for affected stars, avoiding a global directory scan per operation. Validation
of individual records uses exact orientation and duplicate-node checks directly.

Caller live buffers and local validation scratch are included in admission
models before packing/cloning. These are requested-storage estimates, including
map-node estimates, not certified allocator-inclusive bounds or process RSS.
The maximum-work statistic records attempted bounds; the intentional SIZE_MAX
hostile-caller control is an overflow rejection, not an allocation. Hard operating
system OOM recovery is not certified. The existing World exchange handles the
modeled transport admission; real allocation failures inside that house helper
have not been injected here.

The directory certifies metadata/version/storage consistency. It does not
certify arbitrary replacement geometry, metric or global embedding. Callers must
start from unique complete source ownership, reserve global IDs, import complete
operator dependencies and prove geometry/marker/metric approval before publication.
Reservations serialize a synchronous proposal round; they are not persistent
locks suitable for asynchronous migration.

## Validation and inspectable meshes

Actual case folder:
/media/rausa/4TB/SU2_Versions/SU2_NativeIntegrated/integration_evidence/native_3d_core_v1/distributed_controls_v3.

PASS: actual MPI 1/2/4, eight distributed Catch cases per rank. MPI1 also executes
all 37 existing 3D kernel controls: total 45 cases / 2749 assertions. The runner
rebuilds all used house communication and ADT units with HAVE_MPI, links actual
CDistributedSearch failure election using section garbage collection, and never
reuses earlier serial objects or substitutes communication stubs.

Controls cover physical facet zero/high IDs and bounded-round wire transport;
complete six-cell vertex/edge and two-cell face discovery; fresh authoritative
payloads; missing/repeated/oversized features; shared-vertex conflict election;
private metric-qualified body-diagonal split and inverse coarsening with atomic
mesh/directory publication; stale versions, rank veto, modeled memory admission,
reused IDs without version advance, partial coordinate movement, inverted cells,
distributed duplicate cell IDs and a deliberately over-budget star.

Explicit communicator migration/return passes N=M=1/2/4 and N=2,M=1;
N=4,M=2/3. Source ownership and return use deterministic cyclic assignment,
not ParMETIS. Only uniquely owned mesh/version/physical-binding records migrate;
this does not qualify CFD solution/history transfer or runtime reconstruction.

Actual initial, private-refined, coarsened and subset-return records are logged.
An independent exact Python audit passes all 15 frames across the rank matrix:
positive volumes, tetrahedral no-overlap, convex-domain containment, oriented
skin/manifold vertex links, finite original facet coverage/markers, original-P1
metric shape and edge lengths. Each frame has unit volume. Returned records
match the accepted private-refined records exactly.

SU2 grids and original sensor authority are regular files in:
- distributed_controls_v3/mpi1_meshes/
- distributed_controls_v3/mpi2_meshes/
- distributed_controls_v3/mpi4_meshes/

This is one private split and its inverse, not a full adaptation sweep. Fine
initial maximum length 3.4641016151 decreases to 2.8284271247; it still exceeds
1.8. Refined/returned frames explicitly fail full size completion and are audited
as private progress. Coarsened unit-metric grids meet the strict .20/.05/1.8 gate:
minimum mean ratio .7559526299, minimum Jacobian .5773502692, max length 1.7320508076.
Original sensor data used in the independent metric audit is an event fixture;
there is no distributed original-donor query service or transported-field residual
qualification yet. Geometric target composition remains actual-point evaluation
in the field component; unsupported composed edge integration still rejects.

Source/binary/artifact pins, compile and MPI logs, source snapshots and per-rank
profiles are retained in validation.json. The initial distributed_controls_v1
failed only while compiling a test snapshot helper that tried serializing a const
map key; no MPI run started. Its source and error evidence are preserved.
The first passing v2 matrix is also retained; final v3 adds the repeated-build
rejection control. All 15 JSON frames and 15 SU2 grids are byte-identical to v2.

## Cost and scope

One low-priority sequential build and MPI1/2/4 correctness matrix, one numerical
thread. Real host load/process snapshots precede the build and each launch.
Whole MPI launches took .520/.411/.370 seconds in this single observation;
external audits .234/.302/.376 seconds. These include launcher/correctness work,
not adaptation-event cost, repeat timing, rank-choice or scaling evidence.

Directory build/lookup/fetch/reservation/publication timers are mutually exclusive
public-call totals. Directory election time is nested and must not be added to
those totals. World transport counters cover only instrumented collectives,
include direct fixture routing/audit collectives outside directory calls, and
omit the house helper's internal failure elections: world_transport_partial_s
is not total MPI wait or a nested directory closure certificate. Mesh exports
and exact external audits are separate from production operation timings.
Full event closure/private CPU/wait/RSS/instrumentation-overhead measurements
remain required in the actual engine. No speedup or affordability claim.

## Next required work

Implement the distributed original-sensor query/import service, bounded native
3D engine with incremental candidate invalidation, repeated metric completion,
general coarsening/movement/reconnections and adaptable physical surfaces.
The existing CRankBoxTree::Build currently gathers on the full default communicator;
reuse inside M workers requires an explicit-communicator extension and subset tests.
Do not use its current collective accidentally while N-M CFD ranks are idle.
Integrate tetrahedral cost-weighted ParMETIS partitioning/M<=N, native runtime,
accepted mesh/CFD solution/history return and SU2/CGNS lifecycle output. Validate
rotated/spatial anisotropy and rejection on the full MPI matrix, then establish
memory/performance/imbalance and practical size/rank limits. The opt-in runtime
still rejects 3D until the complete implementation is validated; 2D/MMG source,
finer sensor policy, fade and ADAP_HESSIAN_NOISE=0 remain unchanged.

Replay after checking host contention, in a NEW announced folder:
```bash
PYTHONDONTWRITEBYTECODE=1 nice -n 19 python3 \
  integration_evidence/native_3d_core_v1/run_distributed.py \
  integration_evidence/native_3d_core_v1/distributed_controls_NEW
```
The runner prints case/result folders, refuses overwriting evidence, fixes
numerical threads to one, runs jobs sequentially and supports source snapshots
without Git. OpenMPI/mpicxx and the existing repository dependencies are required.
