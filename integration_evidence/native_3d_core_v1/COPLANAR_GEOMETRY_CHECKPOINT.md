# Finite coplanar wall constraints — 2026-10-11

Repository: `/media/rausa/4TB/SU2_Versions/SU2_NativeIntegrated`.
Branch: `codex/native-3d-core`; predecessor `9557213b3bbc8b1036464bc98a5030a84d449261`.
Goal 1 remains ACTIVE with its full runtime/partition/CFD/output/performance scope.

Original triangle seams on one flat physical wall are no longer compulsory
features when the entire connected same-marker component is certified as a
convex finite planar disk. This permits physical triangles to cross those seams
and eliminates an original wall-fan vertex without split history.

## Implementation and geometry argument

`PlanarReference` retains every original facet and marker. It builds shared-edge
components using exact coplanarity and consistent projected orientation, and
reuses the cavity manifold sphere/disk validator. Certification also requires
oppositely oriented internal edges, distinct node coordinates, and every vertex
inside every oriented boundary halfplane. A consistently oriented planar disk
with that convex boundary covers the finite polygon; it does not authorize an
infinite plane or a convex-hull fill of an arbitrary triangle union.

Each certified component uses its minimum original facet ID, independent of input
order. Private surface validation canonicalizes bindings, including inverse-split
sibling pairing. Marker junctions, real creases and disconnected pieces remain
separate. Uncertified concave, holed or malformed components keep their individual
triangle constraints. This is an explicit intermediate capability limit, not a
claim that all simple nonconvex domains are complete.

Point containment is sufficient for a whole face and its affine movement only
because the admitted reference patch is convex. The existing positive endpoint
volume/oriented-face and complete-star checks remain necessary. Rejected geometry
and distributed transactions still leave accepted inputs unchanged. The reference
remains bounded to 256 imported original facets; every worker must have the same
immutable authority. Distributed geometry registries and larger authority sets
remain runtime work.

The streamed repair private CPU timer now encloses only participating local
reconstruction, excluding failure election. Collective wall time remains in its
phase. Full private CPU/wait and production lifecycle qualification is unfinished.
No target, q/J/length gate, sensor-only original P1, cache/composition policy, BL
fade, Hessian noise, 2D/MMG code or production linker option changed.

## Correctness evidence

`coplanar_regression_v2/validation.json` PASS: MPI1/2/4 full previous matrix and
new controls. MPI1 has 73 cases / 4652 assertions; MPI2/4 have 23 cases per rank.
`mesh_identity.json` proves all 1350 prior mesh/source artifacts byte-identical.

`mpi{1,2,4}_coplanar/independent_audit.json` independently audits 21 frames and
18 successful events, including N=M and M<N. Original square-base pyramid:
4 tetrahedra -> 2 -> 28 (one worker) or 30 (multiple workers) -> 2. All final
q >= .20, J >= .05, L <= 1.8. The two-cell mesh has q about .8894 / J .7698;
one-worker refinement has q .6684 / J .4364. Exact volume stays 1/3. The auditor
re-proves supplied groups as finite convex disks, checks oriented marker area,
embedding/manifold/no-overlap and immutable original sensor-P1 shape/edge demands.
Four adversaries include wrong binding, raw seam constraints, cross-marker
merging and convexification of an annulus. C++ controls additionally exercise
wall movement, inverse bisection across the old seams, tiny off-plane/crease
perturbations, an oblique plane, marker vetoes, holes and atomic rollback.

`actual_nonmpi/validation.json` PASS: actual g++ non-MPI build, nine passive
cases / 115 assertions, repeated 6->48->6->48 cycle, automatic/private repair,
strict metric gates and rejected-input/composed-integrator-refusal controls.
The small control process peaks at 5288 KiB RSS; this is not a production limit.

`coplanar_sources_v1/validation.json` PASS MPI1/2/4 and M<N: 63 independently
checked initial/result frames, 36 successful events and six deliberate exhausted
private rejections. The source750 refinement/coarsening/repair queries still
complete. All 126 preceding large-source mesh/state files are byte-identical
(`mesh_identity.json`). Original-P1/query support and seven adversaries pass.
The source2058 query union still requires 294 donors and rejects at the frozen
256 cap. The changed streamed repair timer scope executes in this campaign;
it is not a complete MPI wait/CPU attribution certificate.

`coplanar_regression_v1` is retained as FAIL evidence. Its actual MPI1 meshes
passed; the first wrong-binding adversary chose its existing binding and changed
nothing. The corrected adversary deliberately chooses a different physical
facet. Do not interpret the v1 `_rejected` adversary grid as an engine rejection.

## Performance evidence and limits

`coplanar_repeats_v1/validation.json` PASS: three sequential process repeats for
each of MPI1/2/4. Mesh, accepted/rejected edits and recorded query/import workload
are identical within each rank choice. Per-rank parent phases close against the
parent and include metric preparation, migration, worker communicator, engine,
return and record output. Parent maxima are never sums of phase maxima.
These are shared-host fixture pilots, not fixed-work scaling across partitions.

| One-worker event | Parent rank-maximum median / range | Accepted edits |
|---|---|---:|
| Original wall fan coarsening, 4->2 | .007931 s / .007647-.008739 | 1 |
| Refinement, 2->28 | .023182 s / .022885-.028997 | 14 |
| Coarsening, 28->2 | .036094 s / .035181-.039051 | 14 |

Reference construction is separate: one-rank median .000135964 s for eight
original facets. Maximum requested engine work is about 2.23 MB; this excludes
caller geometry storage and is not RSS. Full useful-edits/second and all rank
choices/ranges are in the receipt. For example, N=4/M=3 final coarsening median
.071687 s exceeds N=1/M=1 .036094 s; more workers do not help this tiny fixture.
Different multi-worker refinement trajectories reach 30 cells, so this comparison
cannot establish fixed-work scaling or motivate a production worker policy.

`actual_nonmpi/previous_pairs/validation.json` PASS: three alternating pinned
baseline/candidate process pairs, each containing three internal OFF/ON controls.
Nine internal samples per setting are not nine independent process repetitions.
Recorded accepted/rejected/edge work matches. The candidate is slower:

| Detailed control | Baseline median | Candidate median |
|---|---:|---:|
| Refinement | .048933618 s | .052064979 s |
| General coarsening | .195026874 s | .209608141 s |
| Repair | .011790592 s | .012340941 s |

Coarsening ranges .192384175-.219286642 / .206411040-.214558786 overlap due to
one baseline outlier; OFF medians .195768790 / .207460354 are also slower in the
candidate. Trace/field/metric object files are byte-identical, but final kernel
addresses shift (`kernel_layout_observation.json`). The earlier checkpoint showed
layout sensitivity; this checkpoint has **not isolated the cause** of its 6-7%
regression. No diagnostic padding or production linker flags were added.
Candidate OFF/ON medians overlap their spreads in these controls; that is not
an instrumentation overhead bound for a production CFD workload.

The repeated fixture reporter counts existing nested search/interpolation/edge
work. It is not a complete query CPU/wait/RSS envelope. ParMETIS, native CFD
rebuild/solution-history return, actual SU2/CGNS lifecycle and R = adaptation/CFD
cost remain unqualified. The production 3D support guard remains intact.

## Inspectable meshes and reproduction

Inspect `coplanar_regression_v2/mpi{1,2,4}_coplanar/`:
`coplanar_initial.su2`, `coplanar_m1_s0.su2` (coarsened wall),
`coplanar_m1_s1.su2` (refined), and `coplanar_m1_s2.su2` (coarsened again),
plus multi-worker variants. JSON carries accepted mesh and immutable original
sensor states. These are manufactured grids, not CFD solution files.

Run sequentially from the repository; each destination must be new:

```bash
python3 integration_evidence/native_3d_core_v1/run_distributed.py /absolute/new/matrix
python3 integration_evidence/native_3d_core_v1/run_coplanar_repeats.py /absolute/new/matrix /absolute/new/repeats
python3 integration_evidence/native_3d_core_v1/run_serial_engine.py /absolute/new/matrix/actual_nonmpi
```

Runners print absolute case folders, pin source/binary hashes and preserve failures.
Publication uses verified aliases for duplicate evidence and excludes ELF/object/
bytecode files. See `coplanar_checkpoint_artifacts.json`. No cluster launch requested.

Next development: extend weighted tetrahedral partitioning and the opt-in actual
native 3D remesher/accepted CFD-state lifecycle, preserving the supported 2D
path. General finite-union boundary coverage and the measured performance
regression remain explicit gaps; none permits closing Goal 1 from these pilots.
