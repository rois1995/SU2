# Full anisotropic MPI refinement matrix checkpoint

2026-10-10. Repository:
`/media/rausa/4TB/SU2_Versions/SU2_NativeIntegrated`.
Branch `codex/native-3d-core`, predecessor
`e920aedd2780c7daa8c47ce1efb3bc311ada9267`.
Goal 1 remains active with its full deliverables. No production 3D enablement.

## Frozen matrix and implementation scope

ANISOTROPIC_MATRIX_PLAN.md defines the matrix and unchanged q>=.20, J>=.05,
original-target length<=1.8 gates before execution. The full Refine engine runs
on thin boxes of heights 1e-4 and 1e-8, a sheared thin box, spatial eigenvalues
with a fixed xy rotation, and spatial eigenvalues with changing nodal directions.
The target is immutable original-tetrahedron componentwise P1 of stored tensors,
including the turning case; evaluating the analytic nonlinear formula at new
points would change the problem. These are already thin manufactured domains,
not BL generation from coarse Euler grids.

Tests migrate both accepted mesh and original sensor records from N to M workers,
exercise actual source import/canonical queries, fully refine privately, and return
mesh records to N. N=1/2/4, M in {N,max(1,N-1),max(1,N/2)}. Each result is independently
checked; different worker partitions can choose different meshes. Accepted parent
input remains byte-unchanged. No sensor-only substitute for a composed target,
target relaxation, normal reset or changed quality threshold is introduced.

This checkpoint changes the MPI fixture, reusable runner and independent auditor;
all native production kernel/engine/metric/2D/MMG implementations remain unchanged.
The auditor validates actual transported tensors and P1 samples, positive exact
volumes, SAT no-overlap, convex domain containment, oriented conforming skin,
sphere/disk links, reference-facet area coverage, markers and metric gates. Three
focused missing/tampered-record controls detect bad source/query observations.

## Final evidence and grids

Final native campaign: `anisotropic_engine_v3/validation.json`, PASS.

- MPI1: 55 cases / 2936 assertions, including all 38 existing 3D kernel cases.
- MPI2: 17 cases per rank / 291 and 320 assertions.
- MPI4: 17 cases per rank / 356, 328, 371 and 342 assertions.
- Thirty completed anisotropic grids and fifteen initial grids across the matrix.
- Actual transported original nodal sensors/coordinates match exactly.
  260 canonical P1 samples have maximum normalized component residual
  9.02058780165053e-17, under the frozen 2e-14 audit tolerance.
- No domain volume changes, overlaps/gaps, missing facet coverage or marker changes.
  Smallest observed metric q=.5441 and J=.2936 remain above the frozen gates.

| Target | M=1/2/3 final tetrahedra | M=4 final tetrahedra | Maximum metric edge |
|---|---:|---:|---:|
| thin4/thin8/shear4 | 48 | 48 | 1.7320508076 |
| spatial | 40 | 42 | 1.7498545258 |
| turning | 44 | 46 | 1.7742354749 |

Single-worker runs accept 19 edits for the aligned/sheared cases, 15 for spatial,
and 17 for turning. M=3/4 includes a small number of rejected private attempts;
all events still complete. These are distinct accepted trajectories, not a
fixed-work speedup/scaling comparison.

Inspect self-contained SU2 grids and corresponding original-metric JSON in:
`anisotropic_engine_v3/mpi{1,2,4}_anisotropic/aniso_{thin4,thin8,shear4,spatial,turning}_m*.su2`.
Initial grids are `*_initial.su2`. Earlier forced one-round rejected grids remain
in `anisotropic_engine_v3/mpi{1,2,4}_meshes/engine_rejected.su2`.

All 90 final anisotropic JSON/SU2 artifacts match the canonical re-audit of v1/v2;
all 54 prior box mesh artifacts match the published engine checkpoint exactly.
`matrix_summary.json` records these matches, per-case/per-rank profiles and gates.

## Profiling and diagnostic corrections

Each matrix event now records an exclusive parent mesh-record cycle:
source+mesh migration, worker communicator creation, worker phase, return to N,
C++ test audit, record export and other. All 85 parent-rank cycles close within
1e-12 against their parent timers. Inactive workers explicitly wait on the
parent completion vote; their phase duration is recorded, not presented as
complete active-rank MPI wait. Source verification and external engine duration
are nested inside worker time. External engine duration includes final object
destruction, unlike the existing engine parent timer. Synthetic nodal tensor
preparation is reported once per case, outside these repeated worker comparisons.

The worker phase includes test-only source observations and reporting. The
post-cycle profile gather/print is excluded from the reported cycle. Independent
Python audits/SU2 export are separate runner stages. No ParMETIS, actual Hessian/
gradation/complexity preparation, CFD state/history return, geometry/solver rebuild,
transfer or production output is present; full adaptation lifecycle qualification
remains required. Nested engine query timings are never added to parent phases.

Two diagnostic faults were found and fixed without native operator changes:

1. Concurrent MPI original-input log arrival changed the order of the auditor's
   JSON donor list. All geometry and tensor datasets were identical; only
   serialization differed. The exporter now sorts by original cell ID.
   `anisotropic_engine_audit_v3` re-audits saved v1/v2 observations and proves all
   90 canonical JSON/SU2 matches, without another native simulation.
2. V2's MPI4 stdout interleaved two long ENGINE lines. Geometry/transport tests
   passed but this incomplete profile cannot support qualification. Fixtures now
   gather bounded diagnostic strings with explicit worker/parent communicators
   and emit them from one rank. The runner rejects interleaved/incomplete records.
   Final v3 profiles are complete. Earlier v2 raw logs and pins are preserved.

Single local pilots give parent cycles roughly .050-.080 s on the final matrix.
The useful engine work and sizes accompany each time. They do not establish
MPI scaling or an optimal M: at these tiny sizes, conflict and communication cost
can outweigh extra workers, and spatial/turning trajectories change with M.
Larger-source/fallback and repeated timing/scaling campaigns remain required.

The reusable runner now audits/exports saved mesh records before rejecting a
nonzero MPI command status, where enough records exist. A small forced-exit7
helper control in `anisotropic_engine_v2/runner_rejection_control` proves record
retention plus independent export of actual saved native rejected records, while
default required-command failure still raises. This is a runner control, not
MPI process-failure recovery or a fresh CFD simulation.

## Replay and remaining work

```sh
python3 integration_evidence/native_3d_core_v1/run_distributed.py /absolute/new_anisotropic_matrix
```

The runner prints full result folders, pins sources/binaries, checks host contention
and uses sequential low-priority MPI1/2/4 with one library thread. Nodes need no Git.
Old logs/snapshots/receipts remain; earlier duplicate grid directories need not be
published because saved actual records support replay and final grids are retained.

Next: repeated coarsening and general constrained collapse/movement/reconnection,
including boundary triangles; larger conforming-source fallback controls; tetrahedral
cost weights/ParMETIS; opt-in runtime/CFD/CGNS/rebuild/transfer; full cost/wait/memory
closure and cluster repetitions/envelope. Existing inverse-bisection/private repair
operators are ingredients, not a completed distributed coarsening engine. The full
Goal 1 is unfinished. No curved geometry, robust BL construction, unsteady 3D or
CFD affordability claim follows from this manufactured matrix.
