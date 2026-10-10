# Planar movement and face/edge reconnection checkpoint

2026-10-10. Repository:
`/media/rausa/4TB/SU2_Versions/SU2_NativeIntegrated`.
Branch `codex/native-3d-core`, predecessor
`92bff69fc1442f0e84eddc81edee85432e5bfccf`.
Goal 1 remains active with its full scope. Production 3D remains guarded.

## Implementation and geometry contract

`MoveVertex` replaces a point in its complete vertex star by a caller-reserved
new identity at a distinct position. Cell/vertex counts stay unchanged. Every
interface face containing the removed point must be physical; otherwise the
star is incomplete. The new point must lie exactly on every incident immutable
finite reference facet. Positive endpoint cell determinants and projected face
areas preserve orientation throughout the affine displacement. Facet-contained
motion, fixed other interfaces and final ball/link/prescribed-skin checks preserve
the embedded old domain. Original corners, marker junctions, facet seams, folded
faces, coincident points, invalid/overflowing IDs and partial stars reject without
changing either output vector. The replacement identity allows physical coordinate
changes while keeping the generic fixed-interface coordinate checks intact.
Global ID reservation and complete dependency admission remain caller duties.

`Reconnect` reuses `Cone` with an existing apex and validates the physical skin
against the immutable reference. It covers face2->3, edge3->2 and edge4->4 cases
without changing boundary geometry, node positions or markers. Geometric validity
alone is not acceptance: the actual original-P1 field and strict new q/J/length
progress checks remain caller responsibilities. No copied external mesher code,
new dependency, metric relaxation or geometric tolerance is introduced.

The geometry precondition is an embedded conforming old mesh. These are bounded
private operators; runtime global input certification and automatic repair
selection/fallback are still pending. Existing `Refine`/`Coarsen` event selection
has not yet been extended to choose these repairs. This checkpoint demonstrates
actual MPI transactions with prescribed proposals, not an automatic repair engine.

A conservative limitation is now explicit: finite original triangle seams remain
constraints even when adjacent triangles share a plane/marker. Manufactured wall
points inside a larger reference facet can move, but arbitrary original planar
surface nodes cannot yet move/coarsen freely across those seams. An authoritative
coplanar patch association with feature/marker preservation is required before
claiming general planar-surface freedom. This remains within Goal1's unfinished
geometry/runtime work; no curved-CAD or BL-generation claim is made.

## Frozen matrix and independent evidence

`repair_operators_v2/validation.json` passes with unchanged before/after source
pins. MPI1:63 cases/3773 assertions (44 kernels+19 distributed cases).
MPI2:19 cases/rank,1457/1128 assertions; MPI4:19 cases/rank,
2041/1413/1727/2056 assertions. Actual non-MPI rebuilt control also passes in
`repair_operators_v2/actual_nonmpi`, including metric-validated movement and
reconnection plus prior refine/coarsen/rerefine and rejection/timing controls.

Three geometry/metric variants: ordinary, height1e-8, and sheared height1e-4.
Each covers a bipyramid2->3->2->3 sequence, octahedral4->4 reconnection, wall-point
movement and interior-point movement. N=1/2/4; M={N,max(1,N-1),max(1,N/2)}.
Face/edge/vertex queries import their complete dependencies across actual owners;
all workers propose the same cavity and reservation admits worker zero only.
Source sensors and mesh migrate from N to M; validated mesh returns to N.

Sensors vary with physical x: each stored base tensor is multiplied by1+0.1*x.
The bipyramid's middle event multiplies normal demand by9 to favor3->2, then
returns to the initial target for2->3. These are different manufactured events;
each event freezes its actual accepted-input P1 field throughout reconstruction.
The changed point queries that source at its actual new position. No frozen target
changes during a rejected or successful proposal.

108 accepted repair proposals (72 reconnections/36 movements) and36 initial meshes
pass independent exact-positive-volume, global SAT no-overlap, convex containment,
oriented conformity/manifold links, finite facet coverage, marker and frozen-P1
metric audits. Final q>=.372093, J>=.213201, max L=1.483240; the frozen gates remain
q>=.20/J>=.05/L<=1.8. Every domain volume is exactly preserved.

702 actual imported donor records match accepted input exactly.936 canonical
queries (one vertex+three interior points per worker/event) have max normalized
residual1.0956711422669239e-16 under the frozen2e-14 tolerance; query points, donor
keys and observed tensors are byte-identical across workers. Each MPI size passes
three missing/tampered source/query controls. Original facet coordinates are
independently checked against the declared manufactured geometry.

Before every real publication, a rank veto and a storage rejection verify both
owned mesh bytes and directory snapshots unchanged:216 intentional publication
rejections. The parent accepted input remains unchanged until its test replaces
it by the validated return. Kernel controls separately exercise wrong planes,
missing stars, corner drift, repeated/coincident points and overflowing IDs.
All54 prior box+90 anisotropic+270 repeated-cycle JSON/SU2 artifacts remain
byte-identical to `general_coarsening_v2`. Duplicate earlier grids are omitted
from the commit. `repair_summary.json` retains matches and actual profiles.

Inspect `repair_operators_v2/mpi{1,2,4}_repairs/repair_{face,edge4,wall,interior}_
{unit,thin8,shear4}_m*_s*.su2` (filename pattern wrapped here only); each corresponding
JSON contains geometry authority, original donor mesh/tensors and result mesh.
Face s0/s2 are2->3, s1 is3->2; the other cases have one event. Original `_initial`
files are retained. All exported dependencies are regular local files.

## Costs and limits

All306 parent fixture profiles close against preparation, migration, worker
communicator, worker execution, return, test audit/export and other. Input
observation output is charged to other. Worker time includes source verification,
private geometry/metric work, both rejection controls and publication. Private
reconstruction/metric and nested donor search/interpolation/edge-tracing counters
are reported separately. Communication bytes/collectives and separate directory/
donor requested-byte models are recorded. These models do not include all live
caller storage simultaneously and are not RSS/OOM certification. Inactive-worker
time measures completion-vote wait, not a full active-rank MPI wait decomposition.
No sum of rank maxima is claimed as elapsed time.

These are small single-patch pilots with deliberately serialized overlapping
proposals, not balancing/scaling or large-mesh repair benchmarks. They establish
that MPI protocol and geometry/metric gates agree. Full automatic selection,
accepted work rates/CPU/wait/imbalance, complete memory and practical limits remain
required. Exact original-source edge tracing remains the known dominant field
cost from the earlier event campaigns; new operator data does not erase that cost.

Actual non-MPI three-repeat OFF/ON controls preserve prior event meshes and work:
refinement medians .05566/.05738 s, ranges .05143-.05742/.05256-.06157;
coarsening .21990/.21353, ranges .21972-.22137/.21186-.21479. This unresolved
workstation variability does not establish instrumentation overhead or a speedup.
These triples cover the existing full refinement/coarsening events, not new repair
operator overhead in isolation. No CFD affordability claim follows.

V1 native MPI1 passed63 cases/3773 assertions but its audit parser shadowed the
worker-count variable with the sampled metric tuple. Saved-log re-audit passes30
then-constant-target frames and three tamper controls. V2 corrects the parser,
adds spatial sensor strength/interior query observations and worker consistency.
Raw failure/source pins and re-audit evidence are retained; no original native
mesh was rerun merely to repair that audit failure.

## Next work and replay

Automatic bounded repair selection and rejected-operation fallback are next,
including measured original-edge cache reuse across candidates. Test genuine
shape/sliver stalls, partial cavities surrounded by other cells, larger-source
batch fallback and repeated mixed edits. Add coplanar patch associations that
preserve true features and markers while freeing artificial original seams.
Then weighted tetrahedral ParMETIS, actual opt-in remesher/CFD solution/history
rebuild, SU2/CGNS runtime output, 2D regression and full cost/memory/scaling/cluster
qualification remain required. Goal1 is not complete.

Replay: run `run_distributed.py` with a fresh absolute destination after a real
host contention check, then `run_serial_engine.py` with a fresh subfolder of its
PASS result. Runners use one compiler at a time, one library thread and MPI<=4;
source/binary/artifact pins, failures and inspectable grids are retained.
