# In-event tetrahedral repair recovery checkpoint

2026-10-11. Working repository:
`/media/rausa/4TB/SU2_Versions/SU2_NativeIntegrated`.
Branch `codex/native-3d-core`, predecessor
`2bd703c513577fb0f9a7ea6d8e8c8b73c6d61303`.
Goal 1 remains active with its full scope. Production 3D remains guarded.

## Resulting behavior

A rejected refinement proposal can now try bounded fixed-skin reconnection before
blocking its edge. A rejected coarsening proposal can try movement/reconnection
when the current mesh violates the frozen metric gates. An already-admissible
coarsening mesh retains ordinary short-edge removal, avoiding repeated
quality-only rearrangement. Coarsening still makes no minimal-cell-count promise.

When refinement/coarsening stagnates with invalid cells and no admitted edge
candidate, the existing event activates a failing-feature repair queue once.
The queue admits complete vertex/edge/face stars, reuses immutable original-P1
sensor imports and source/directory indexes, and publishes metric-improving
movement/reconnection through the same atomic path. Normal edge queues remain
current during recovery; coarsening resumes after all cells become admissible.
The original round limit covers primary edits and recovery together. There is
no detached remeshing call, donor resampling, source rebuild or target relaxation.

New counters distinguish fallback attempts/acceptances and recovery activation
from split/collapse histories and movement/reconnection. Routing uses existing
admitted parent owners for a repaired patch; accepted input remains immutable.
Fresh q>=0.20/J>=0.05 and frozen progress gates remain strict, and full-event
completion requires every metric edge<=1.8. Finite planar facets/markers remain
authoritative. Coplanar original triangle seams still constrain motion.

## Performance controls introduced with recovery

The scalar completion reduction distinguishes admissible fields, remaining
coarsening candidates and actual invalid metric cells in one existing collective.
It does not add a separate reduction to every ordinary coarsening round.
A one-time recovery queue build is charged to selection and admitted with a
conservative4096-byte-per-cell temporary budget. Steady queue memory accounting
remains incremental; bounded donor/index/query storage is unchanged.

Movement proposals first check exact incident-facet membership, so an obviously
immovable corner can reject before reconstructing its entire star. The complete
geometry operator still validates every proposal that passes this preflight.
Private metric changes reject failing shape/sliver criteria before expensive
source-resolved edge integration. The new composition-aware kernel control proves
four actual centroid compositions still occur while zero edge integrations are
performed for a known bad-shape proposal. Ordinary seed/final measurement keeps
its complete edge measurements and strict gates.

V1's broad coarsening fallback policy passed correctness but performed
unnecessary quality edits on already-admissible fields. Several serial repeated
coarsening events grew to0.4–1.4s with additional accepted reconnections and
changed intermediate trajectories. V2 restricts this work to invalid fields.
The serial regular matrix returns to zero fallbacks and the earlier accepted
operation counts, with event times about0.20–0.25s in this contended run.
This is a policy/work comparison, not a fixed-work speedup estimate. Both pilots'
raw logs, source snapshots, audits and per-rank comparisons are retained.

## Evidence

`recovery_integration_v2/validation.json` and
`recovery_integration_v2/actual_nonmpi/validation.json` PASS with stable source pins.
MPI1:65 cases/4493 assertions; MPI2:20 cases/rank,2020/2773 assertions;
MPI4:20 cases/rank,2521/3187/3976/3853 assertions. Actual non-MPI exercises
recovery through both entry points, one-round exhaustion with accepted-state
preservation, prior private operators and repeated refine/coarsen/refine controls.

Three variants (ordinary, h=1e-8, sheared h=1e-4), four failure mechanisms and
all worker subsets are exercised through Repair, Refine and Coarsen. The matrix
completes216 events, independently audits324 meshes including108 initial meshes,
and closes612 parent profiles. Eighteen actual private fallback acceptances fix
the constrained coarsening/sliver case. Refinement restores wall/interior shapes
through the shared recovery queue; coarsening also restores the size-only
reconnection case. The auditor verifies the required recovery/fallback counters,
not just a successful result flag. All nine missing/tampered-data controls reject.

Independent checks cover exact positive volumes, no positive-volume overlaps,
conforming incidence and sphere/disk links, convex domain containment, oriented
finite-facet coverage/markers, original-P1 shape/integrated lengths, and agreement
with native final q/J/length reports. Deliberately bad initial shapes are measured
and geometrically audited; accepted final criteria are never relaxed. Worst final
q=0.372093, J=0.095473, L=1.483240. The earlier actual transported/P1 query audits
also rerun in the shared field core; this is not a new per-candidate observer.

All918 earlier JSON/SU2 artifacts match the published automatic-repair checkpoint
byte-for-byte, including refinement, anisotropic refinement, repeated general
coarsening, prescribed repair and original direct-Repair trajectories. New grids
with `restoreR_`/`restoreC_` prefixes are retained; old duplicate grids are omitted
from Git while their fresh audits and raw records remain.

The fixture envelope observed at most35 rounds and2,166,904 modeled requested
work bytes. It is not a general size/rank or RSS qualification. Source preparation
is outside repeated subset-event timing; ParMETIS, real CFD reconstruction,
solution/history transfer and full collective wait are still absent.

Actual non-MPI instrumentation OFF/ON medians, three alternating pairs each:

| Event | OFF | ON |
|---|---:|---:|
| Refine | 0.048458s | 0.048176s |
| Coarsen | 0.194024s | 0.193663s |
| Direct repair | 0.011268s | 0.011550s |

The comparisons assert identical grids, accepted counts and query work. Individual
samples remain in `instrumentation_summary.json`; workstation contention and
three pairs prevent a general overhead/scaling/CFD-affordability conclusion.

## Inspect and replay

`recovery_integration_v2/mpi{1,2,4}_automatic_repairs/restore{R,C}_*_m*.su2`
and `.json` contain the new meshes and their immutable original sensor fields.
`*_initial.su2` contains each intentionally bad starting mesh. The source/binary
pins, raw logs, independent reports and the V1 policy pilot are preserved.

```sh
python3 integration_evidence/native_3d_core_v1/run_distributed.py /absolute/new/case/folder
python3 integration_evidence/native_3d_core_v1/run_serial_engine.py /absolute/new/case/folder/actual_nonmpi
```

Use a new folder, one sequential low-priority job and a fresh actual host check.
No overnight cluster launch is requested. The shared runner announces every folder.

## Remaining Goal 1 work

Exercise multiple and partial cavities and larger source imports; continue from
measured proposal/tracing costs rather than adding unbounded retries. Add
coplanar geometry-patch authority while preserving real features/markers. Complete
cost-weighted tetrahedral partitioning, runtime/mesh/solution/history return to N
CFD ranks, SU2/CGNS and full lifecycle/cluster qualification. The production
native remesher still rejects3D; 2D/MMG/BL/fade/Hessian code was not edited here.
No full Goal1, curved-CAD, robust-BL or unsteady-affordability claim is made.
