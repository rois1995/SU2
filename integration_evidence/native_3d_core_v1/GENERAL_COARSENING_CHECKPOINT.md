# General planar coarsening and repeated MPI cycles

2026-10-10. Working repository:
`/media/rausa/4TB/SU2_Versions/SU2_NativeIntegrated`.
Branch `codex/native-3d-core`, predecessor
`badd73ba3a9261c4e9aa69d152a6f7ee3d6282c0`.
Goal 1 remains active with its full original scope. Production 3D remains guarded.

## Change and geometric argument

`CollapseVertex` contracts a vertex to an existing neighbor using its complete
vertex star. It does not require split history: manufactured interior and wall
points unrelated to bisection are covered by unit controls. Cells containing both
endpoints disappear; remaining cells reconnect to the retained endpoint. Boundary
triangles change with the volume patch. The retained point must belong to every
incident immutable finite reference facet, preserving corners, facet seams and
marker junctions. Incomplete stars, inversion, folded/duplicate faces, bad manifold
links and empty replacement volumes reject without changing the caller's output.

The geometry precondition is an embedded conforming old mesh. Contracting the
point along its edge makes each surviving cell determinant and each changed
planar face's projected area affine in time. Strict positive endpoint signs
prevent intermediate inversion; cells containing both endpoints shrink to zero.
Facet containment keeps boundary motion within the original finite convex facets;
other patch interfaces stay fixed. The prescribed skin and ball/link checks
validate the final replacement. This uses exact signs and avoids subtracting
rounded total volumes in thin domains. It is not a runtime certificate of arbitrary
unvalidated input embedding; independent whole-mesh audits remain essential.

`Coarsen` shares refinement's incremental selection, original donor freezing,
dependency admission, full-vertex reservation, bounded routing, atomic directory
publication and rejection path. It imports the complete removal-vertex star,
tries both directed short-edge candidates, and inherits the surviving parent
owners. Rejected directions are removed until adjacent topology changes.
The frozen proposal threshold is L<0.9, versus refinement L>1.8. Every new cell
still passes q>=0.20, scaled J>=0.05; coarsening introduces no oversized new edge.
Final completion checks the whole mesh against L<=1.8. Existing unchanged oversized
edges may survive a private local proposal, but cannot pass final completion.
Success exhausts the available admitted proposals; it does not prove a minimal mesh.
Cell-ID reservation is overflow checked; coarsening reserves no new node IDs.

No original sensor target changes within an event, no coarse-cell spreading of
composed wall tensors, and no sensor substitute for unsupported geometric edge
integration. 2D/MMG, BL/fade/noise policies are unchanged. A callback is evaluated
at actual field queries; composed 3D edge integration still rejects as unsupported.

## Frozen campaign and evidence

`general_coarsening_v2/validation.json` passes, with stable before/after source pins:
MPI1 59 cases/3319 assertions; MPI2 18 cases/rank (699/848 assertions);
MPI4 18 cases/rank (876/1144/1010/1159 assertions). Strict-warning kernel compilation
and actual passive MPI transports are used. MPI1 includes 41 kernel cases.
Actual non-MPI rebuild/control passes in `general_coarsening_v2/actual_nonmpi`.
Round-limit, storage and mixed-rank operation-intent failures preserve accepted
input bytes. General face-interior/wall/interior collapse controls include
height 1, 1e-4 and 1e-8 domains, wrong facets, missing stars and immutable outputs.

The matrix is ordinary/thin/sheared boxes plus fixed-rotation spatial sizing and
changing nodal directions, with N=1/2/4 and M in {N,max(1,N-1),max(1,N/2)}. Every
subset executes two refine/coarsen cycles, returning mesh records to N after each
event. Each event samples a new manufactured nodal field on its accepted input;
coarsening uses one-quarter of the fine tensor. The field remains original-P1 and
immutable during that event, including the nonlinear analytic turning case.
This is a deliberately changing-target cycle test, not weakening a stalled target.

120 completed event meshes and 15 initial meshes pass independent exact positive
volume, SAT no-overlap, convex-domain containment, oriented conformity, manifold
links, facet coverage, marker, original-P1 shape and metric-length audits.
6,746 actual imported donor records match the accepted-input sensors/coordinates
exactly. 1,040 actual canonical queries have max normalized residual
9.02058780165053e-17 (frozen tolerance 2e-14). Three missing/tampered source/query
controls pass per MPI size. Worst completed q=.54411682, J=.29359336, L=1.77423547.

Ordinary/thin/sheared trajectories are 6->48->6->48->6. Spatial refinement yields
40 cells for M<=3 and42 for M4; turning yields44 or46 depending on the partition
and cycle. All coarsening events return six cells. Different trajectories are
independently audited, not required to be byte-identical. All54 prior box and90
prior anisotropic JSON/SU2 artifacts remain byte-identical to the published
`anisotropic_engine_v3` checkpoint; their duplicate grids are omitted from this
commit. `cycle_summary.json` records matches, trajectories, gates and profiles.

Inspect `general_coarsening_v2/mpi{1,2,4}_cycles/cycle_*_m*_s*.su2`;
each corresponding JSON contains the original donor mesh and nodal sensor tensors,
accepted mesh, facet authority and markers. s0/s2 are refinement, s1/s3 coarsening.
The existing refinement `engine_rejected.su2` remains available at the prior
published checkpoint. Synthetic coarsening rejection controls verify state
preservation; this campaign contains no unexpected rejected event mesh.

## Costs, overhead and limits

All340 parent phase profiles close: nodal preparation, mesh+sensor migration,
worker communicator, worker phase, return, test audit/export and other.
Input observation output is charged to other. Source verification, rejection
controls and external engine time are labelled nested within worker time.
Inactive-worker phase time is completion-vote wait, not a full active-rank MPI
wait decomposition. The engine retains exclusive selection/dependency/reservation/
donor/reconstruction/metric/routing/publication/update accounting, actual query
subcosts and private CPU. Parent profiling excludes final profile-line collection.

The MPI1 first box coarsening accepts19 edits and rejects9 in28 rounds. It measures
129 selection edges and reuses555; total field edge integrations270. Its .2181 s
engine pilot includes .1751 s nested exact source-edge tracing versus .0000603 s
sensor interpolation. Coarsening's original source has48 tetrahedra versus6 for
initial refinement. Exact tracing is the dominant optimization target before
larger source/size campaigns. Maximum observed rounds79, modeled work2,374,600
bytes; these are observations, not proven practical size/rank or RSS limits.

Three matched actual non-MPI OFF/ON controls preserve mesh bytes and work:
refinement OFF median .05207 s (range .05165-.05326), ON .05116 (.05112-.05199);
coarsening OFF .21490 (.20870-.21631), ON .20962 (.20925-.21410). Detail overhead
is unresolved within host variability; no zero-overhead or MPI scaling claim.
Coarsening controls accept19/reject9 with270 field edge calls, versus19/234 for
refinement. Observed process RSS5,052 KiB for this tiny sequential control is not
memory qualification. MPI cycles are single pilots on a contended workstation.

Raw `general_collapse_v1` preserves the earlier wrong test expectation about
unchanged oversized edges. `general_coarsening_v1` native MPI1 passed59 cases/3210
assertions; its wrapper then failed on an accidental extra audit argument.
A separate independent re-audit passes all15 then-existing constant-target cycle
frames. Final v2 corrects the wrapper and expands the frozen matrix. Logs, source
snapshots and receipts are preserved, without duplicate old grids/build binaries.

## Replay and next work

Run the reusable sequential runner with a fresh destination, after checking host
contention, e.g. `nice -n19 python3 integration_evidence/native_3d_core_v1/run_distributed.py
/absolute/new/case/folder`. Then run `run_serial_engine.py` with a fresh subfolder
of that completed PASS destination. Python3.8, MPI and Eigen/Catch headers suffice;
these controls do not build or run CFD. Detailed timing is opt-in.

Movement and bounded face/edge reconnection, larger-source/batch fallback controls,
cost-weighted tetrahedral ParMETIS, actual opt-in remesher/runtime/solution/history
rebuild, SU2/CGNS runtime output, 2D lifecycle regression, total MPI wait/RSS and
cluster correctness/scaling/affordability remain required. No robust BL creation,
curved geometry, full unsteady 3D or affordable-CFD claim follows from this checkpoint.
