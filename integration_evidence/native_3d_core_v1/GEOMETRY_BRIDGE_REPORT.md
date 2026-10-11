# Actual native 3D CFD-owner bridge checkpoint

Both `geometry_bridge_v10/validation.json` and `geometry_meson_v1/validation.json`
are PASS. The latter uses actual configured Meson test objects and complete current
Common/strict/CFD archives, without standalone native/house override objects.
The Common/strict/CFD rebuild in `geometry_build_v1` passed 268 steps with no drift.
This publishes helpers and bridge tests; production native3D driver dispatch is
still guarded and full Goal1 remains active.

## What passed

MPI1/2/4, every M=1..N, both barycentric and conservative transfer, three changed
sensor-target events per combination: 42 adaptation/return events per matrix.
Actual CFD-owner sensor-only tetrahedra and halo coordinates are imported; weights
and explicit worker partitions are built; the core refines/coarsens; distributed
readers return to N and actual geometry/solvers are rebuilt. Immutable finite
planar marker authority survives reader renumbering and repeated events.

Each independent matrix audit covers 84 mesh frames,42 donor/accepted state pairs,
42 real SU2/CGNS writer/reader round trips, exact positive volume1, face/edge/vertex
manifold and reference-marker conformity, exact overlap tests and original-P1
integrated metric edges. Raw SU2 and independent HDF5 CGNS parsing check positive
tetrahedra, geometry/connectivity, seven marker sections/BC ranges and closed links.
Current and time_n hold the same end-step U^n; time_n1 is distinct and transferred.
The independent state checker rejects mismatched time_n as well as other corruptions.

Worst audited values are identical for both matrices:

| Quantity | Observed | Frozen gate |
|---|---:|---:|
| Mean-ratio quality | 0.3679273892 | >=0.20 |
| Scaled Jacobian | 0.1463819359 | >=0.05 |
| Original-P1 metric edge length | 1.7937419461 | <=1.8 |
| Transported sensor absolute residual | 0 | <1e-13 |
| Current/history relative residual | 8.471971e-13 | <2e-10 |

Reader-budget refusal leaves its sentinel output and donor CFD state unchanged on
all ranks. Original sensor records and accepted donor current/history bytes remain
unchanged throughout. A manufactured thin query hook verifies every cached/remote
weight and reader query composes at the actual point and preserves finer interior
sensors. It does not establish composed3D edge integration or BL generation.
The shared geometry fixture's 2D barycentric/conservative history regression passes
on MPI1/2/4 using existing transfer test mesh sizes (n4 donor,n6 target,MG requested1).

## Inspectable outputs

Actual configured-archive outputs are in `geometry_meson_v1/mpi{1,2,4}`.
For example `mpi4/bridge_barycentric_m4_s2_written_SU2.su2` and its `_CGNS.cgns`
partner contain the 100-cell accepted third-event grid. The matching
`bridge_barycentric_m4_s2_states.json` retains donor/accepted current/BDF2 states
and transported sensors. `bridge_barycentric_m4_s2.json` retains original-P1 donors
and immutable facet authority. These are manufactured cube cases, not real-flow
simulation or viscous/unsteady CFD validation.

## Performance scope and remaining work

The archive test launch durations were 1.044s/2.506s/9.137s for MPI1/2/4. They include
assertions, both transfers, mesh outputs/readbacks and differing numbers of worker
choices. Python audits are timed separately. One contended repetition supplies
diagnostics only, no speedup, rank-efficiency or affordability claim. Import,
weight and reader timings are retained, but complete production lifecycle phase/
CPU/wait/RSS closure is still missing.

| N | M | Cell counts after refine/coarsen/refine |
|---:|---:|---|
|1|1|48 / 6 / 48|
|2|1|48 / 6 / 48|
|2|2|48 / 10 / 100|
|4|1|48 / 6 / 48|
|4|2|48 / 6 / 52|
|4|3|48 / 10 / 100|
|4|4|48 / 10 / 100|

These trajectories are not fixed-work scaling comparisons. The next integration
must wire the opt-in 3D remesher, retain durable planar authority and accepted-state
publication, and account for setup/index/import, partition/migration, private core,
return, actual CFD partition/rebuild/transfer/output and rank waiting without double
counting. Complete bounded memory/RSS, instrumentation OFF/ON and repeated timing/
practical size/rank pilots remain; SGE runs can be prepared without requesting
nighttime launches. Preserve 2D/MMG and all existing metric guards.

## Retained failures and limits

V1..V6 are compile/link/helper setup failures. V7 config missed ADAP_SENSOR;
abort_v1 proves its CConfig failure before adaptation. V8 reached adaptation and
reader return, then the synthetic unequal current/time_n fixture violated the
existing transfer contract (abort_v2). Both fixtures corrected, no production
config/transfer relaxation. V9's 3D checks/output audit pass separately; its whole
campaign remains FAIL because the six-point 2D fixture left empty CFD partitions
and aborted in CPoint::SetPoints. The supplemental audit explicitly says 3D-only.
Changing that regression to established fixture sizes avoids this pre-existing
underfilled-CFD-rank limit; arbitrary empty CFD ranks are not qualified.

Native bridge facet authority is bounded at256 input triangles; donor union256,
complete cavity64, bounded query batches and caches remain. Source embedding is an
accepted-CFD precondition independently checked for these cases. Requested native
storage estimates are not allocator/RSS hard ceilings and exclude CFD/reader/
partitioner internals. Import is read-only but still needs complete memory admission
and nonfatal invalid-input handling before full production qualification. No curved
geometry, robust BL construction, full unsteady3D or affordability claim.
