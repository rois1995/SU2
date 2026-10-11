# Weighted tetrahedral working partitions — 2026-10-11

Repository: `/media/rausa/4TB/SU2_Versions/SU2_NativeIntegrated`.
Branch: `codex/native-3d-core`; predecessor `32dfde2bd01f81a9c90bbd1452bbdfbd307a7d02`.
Goal 1 remains ACTIVE with its full production import/return/CFD/output/performance scope.

The native tetrahedral core can now repartition working cells with (Par)METIS,
balance predicted adaptation demand and cell storage independently, run inside an
explicit worker subset, and return adapted records to the parent communicator.
This checkpoint establishes that partition foundation; the production native
remesher still rejects 3D until its actual geometry/state lifecycle is connected.

## Implementation

`CNativePartition3D` uses triangular-face dual adjacency. Sparse 64-bit cell/node
IDs are never cast to the partitioner's graph index: rank-prefix contiguous
ordinals define the graph. Empty graph ranks are excluded from the partitioner
communicator; one nonempty owner uses METIS, multiple owners use ParMETIS.
Every collective uses the supplied `World::comm`, including a second repartition
inside M workers while inactive parent ranks remain outside it.

Two constraints balance work weights and cell count. The tetrahedral estimate
uses sensor-only centroid metric volume relative to a unit regular tetrahedron:
`q^(3/2) * rms_edge^3`. Actual composed centroid and four vertex queries add
refinement/coarsening and shape/sliver demand. Five queries per cell and batches
of at most 128 bound sampling; weights are positive integers capped at 1024.
The static proxy is not a calibrated reconstruction-CPU predictor. Geometric BL
samples never become volume-interpolated wall tensors. Cache hits still compose
at the actual query; frozen target, finer sensor policy, fade and noise stay intact.
A manufactured thin wall demand raises weights modestly, while a genuinely fine
sensor tensor retains its large volume demand. This is a field-policy control,
not a certificate of actual 3D BL construction.

Input validation rejects inconsistent controls, duplicate cell IDs, inconsistent
coordinates for a shared node ID, nonpositive cells, unmarked exterior faces and invalid/nonmanifold interior
faces. Migration stages complete cell records and validates common admission,
identity/count/work before swapping accepted ownership. IDs, versions, every
coordinate and physical-binding field (including unused payload) are preserved
byte for byte. An initial matching donor copy can follow returned destinations;
subsequent donor ownership need not equal working-cell ownership. Original
sensor and CFD solution/history stores are caller-owned and untouched. Untouched
stores alone do not establish successful solution transfer onto an adapted mesh.

Requested native storage includes graph, directory, transport, staging, the old
and replacement ownership maps, partition controls and caller allowance. It does
not cover allocator overhead, RSS or internal (Par)METIS workspace; no complete
hard memory cap is claimed. Graph/directory/staging destruction is included in
the partition parent timer. Graph, partition, migration and residual overhead
are exclusive; directory time is nested inside graph preparation. Collective
wait/CPU are not separately attributed at this new layer yet.

The normal SU2 Meson strict library now receives its MPI/ParMETIS include
dependencies and compiles the partition implementation. Strict arithmetic options
remain last, after global fast-math flags. The new tests are in the normal test
source list. No supported native 2D or MMG algorithm, target or configuration
was changed.

## Correctness evidence

`partition_controls_v1/validation.json` PASS establishes the initial MPI1/2/4
controls. V2 closes imported-auditor source pins and native partition-control
array admission, adds independent auditor adversaries and actual nonMPI checks.
V1 remains historical evidence and its source snapshots are retained.

`partition_controls_v2/validation.json` PASS: three sequential process repetitions
on MPI1/2/4. Four cases per MPI rank; MPI1 has 1244 assertions, MPI2 totals 2574,
MPI4 totals 5492 in each repetition. Actual g++ nonMPI has three cases / 1236
assertions. The 384-tetrahedron cube exercises three initial layouts (one owner,
one empty rank, interleaved) and M=1/N/(N-1)/(N/2) as distinct available values.
Across repeats, 63 partition cases plus three nonMPI cases preserve the complete
records, total 384 cells / 960 prescribed work and both declared load bounds.

The independent auditor decodes the actual scalar wire records, proves the
original grid is an exact Kuhn tiling of the cube, checks orientation and oriented
triangular-face incidence, and reconstructs actual cut, moved-cell and per-rank
work/storage counts. Four explicit adversaries (changed wire record, missing
record, out-of-subset destination, wrong cut count) are rejected in every final
audit. C++ controls additionally veto rank-specific storage and invalid control,
weight, face, coordinate and duplicated-geometry inputs without ownership change.

There are 21 completed actual weighted/subset refinement-and-return events and
42 independent initial/result frames across the three repetitions. Every unit
box goes 6->48 tetrahedra with 27 points and 48 physical triangles; exact volume
is 1, minimum q=.7559526, minimum J=.5773503 and maximum metric length=1.7320508.
Original-P1, marker area, embedding/manifold/no-overlap and strict q/J/L audits
pass. Original donor records remain byte-identical. JSON retains mesh and frozen
sensor states; these are manufactured metric cases, not CFD solutions.

`partition_meson_v1/validation.json` PASS: actual SU2 Meson strict archive and
actual Meson partition/2D geometry test objects, linked with source-pinned unchanged
house objects. MPI1 has 25 cases / 9820 assertions; MPI2/4 have 25 cases per rank.
All new independent partition/mesh audits pass. Fourteen actual compile commands
are retained: twelve strict library objects and two test objects. This qualifies
the changed build wiring and relevant 2D geometry controls, not a full new 2D CFD
matrix or AD/mixed-precision build.

## Performance and practical limits

`partition_controls_v2/repeat_summary.json` records three samples per rank/worker/
layout choice, spread, actual critical-rank exclusive phases and requested native
storage. Mesh/source files and graph/ownership/load work are identical within each
rank/worker choice. Do not sum phase maxima and call that elapsed time. A foreign
four-rank unsteady simulation was active during V2; host load/memory/process
snapshots are retained. These are diagnostic pilots, not speedup or scaling proof.

One-owner initial layout, partition parent rank-maximum median / range:

| Parent N | Worker M | Seconds |
|---:|---:|---:|
| 1 | 1 | 0.005042 / 0.004231–0.006525 |
| 2 | 1 | 0.006608 / 0.004609–0.019390 |
| 2 | 2 | 0.008000 / 0.007663–0.008366 |
| 4 | 1 | 0.022793 / 0.004617–0.023664 |
| 4 | 2 | 0.021720 / 0.015727–0.084794 |
| 4 | 3 | 0.024062 / 0.020936–0.071822 |
| 4 | 4 | 0.034161 / 0.022332–0.065959 |

Largest requested native bound in the displayed controls is about 1.09 MB; it is
not RSS. Directory/graph work and partition-collective waiting dominate different
contended samples. No benefit from more ranks or calibrated work weights is
claimed for this tiny case. Full import/query/reconstruction/return/rebuild/
solution/history timing, private CPU/wait, instrumentation overhead, actual CFD
cost ratio and larger size/rank limits remain Goal 1 work. The preceding 6–7%
nonMPI checkpoint regression also remains unisolated; this additive partition
checkpoint does not claim to fix it.

## Inspectable evidence and next step

New grids and original metric states are retained at their exact names in
`partition_controls_v2/mpi{1,2,4}_repeat1_meshes/engine_m*_initial.{su2,json}` and
`engine_m*_adapted.{su2,json}`. On a checkout, duplicate artifacts resolve through
`partition_checkpoint_artifacts.json`; the first-repeat named grids are actual
retained files. Logs, receipts, source snapshots and actual build commands are
kept; binaries/object files/bytecode are excluded from publication.

The reusable standalone check is:

```bash
python3 integration_evidence/native_3d_core_v1/run_partition.py \
  /absolute/completed/coplanar_regression_v2 \
  /absolute/new/partition_controls
```

It reuses unchanged source-pinned MPI and actual_nonmpi core objects, pins their
current byte hashes and the (Par)METIS libraries, and preserves failures. It is
not a self-contained rebuild of those prerequisites; their compiler commands and
receipts are retained in the prior checkpoint. The actual Meson evidence includes
its runner snapshot and complete commands. No cluster launch was requested.

Next implementation is the opt-in actual native3D adapter: import owned tetrahedra
and immutable faceted marker authority, keep original sensors independently from
working partition, return accepted reader slices on N ranks, and use SU2's existing
geometry/solver reconstruction and direct-original solution/history transfer.
Validate actual SU2/CGNS output and rejection lifecycle before claiming runtime
3D support. Preserve the 2D path, strict gates and actual-query geometry policies.
256-facet/256-donor caps, composed3D integrator refusal and nonconvex finite-union
geometry remain explicit limits. Full Goal 1 is not complete from this checkpoint.
