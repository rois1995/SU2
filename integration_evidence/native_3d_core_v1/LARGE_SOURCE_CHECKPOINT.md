# Native 3D: larger immutable donor sources

Branch `codex/native-3d-core`; predecessor
`7bf0eaa1c2b49edeeb3ea2d17daec1c513f743d4`. Goal 1 remains ACTIVE/full scope.
Working repository: `/media/rausa/4TB/SU2_Versions/SU2_NativeIntegrated`.

## Problem and resulting behavior

The previous importer requested every original tetrahedron whose bounding box
intersected a working cell. A six-tetrahedron cube over a connected 384-cell
original source therefore rejected before measuring its first cell: each coarse
cell box covered the whole source, exceeding the fixed 256-donor patch cap.
`large_sources_v1` records that actual MPI1/2/4 failure and input-byte preservation.

`DonorImport::ImportCells` now routes actual positive tetrahedral query geometry.
When the global original source exceeds the patch cap, owners exclude donors
strictly outside any query face plane or vice versa. Every sign uses the existing
exact binary64 orientation predicate; zero signs remain eligible. This is a
conservative filter, not a complete intersection or source-coverage certificate.
Canonical P1 queries and exact source-edge tracing still decide coverage.

The engine uses cell-shaped imports both for incremental measurements and for
complete admitted private cavities. Original sensors, finer demands, actual-point
composition and immutable source/index reuse remain unchanged. No composed tensor
is transported, metric softened or donor/segment cap raised. Accepted caller input
remains immutable. Fixed-size request records preserve the shared transport contract;
the common 2D transport itself is unchanged.

A source with at most256 global donors cannot overflow the patch cap, so it keeps
the cheaper box path. Source Build computes a capped uint64 sum instead of its
former boolean nonempty reduction: each rank contributes min(count,257), enough
to distinguish empty/small/large sources without summing unbounded counts. The
policy is frozen consistently across workers. Cell request geometry is validated
even when its small-source filter is skipped.

## Verified evidence

Final large-source receipt: `large_sources_v4/validation.json` PASS/stable pins.
Connected sources have162,384 and750 original tetrahedra. MPI1/2/4 use worker
sets M={N,max(1,N-1),max(1,N/2)}. Mesh and original sensors migrate to M; mesh
records return to N. These are manufactured records, not CFD solution/history.

- 12 successful events: six already-admissible coarse cubes and six actual
  single-tetrahedron wedge refinements, each1->8 cells/six accepted splits.
- Six source750 events reject within the original import cap and preserve
  accepted-input bytes. Their meshes are themselves geometrically and metrically
  admissible; rejection is a field-import limit, not mesh infeasibility.
- 27 independently audited frames: nine actual initial, twelve accepted and six
  rejected mesh records. Rational volume/incidence/link/convex containment,
  pairwise SAT overlap, finite facet/marker area and original-P1 q/J/edge checks
  pass. Final wedge q>=.7559526, J>=.5773502, L<=1.7957881.
- 9,828 independent rational full44-axis closed-intersection SAT checks prove
  that the declared filtered imports omit no touching donors. Source384 imports
 170 donors per coarse tetrahedron rather than384. Source162 deliberately keeps
 all162 donors. 144 canonical original-P1/actual-composition observations pass;
 cache-hit composition is asserted in C++ too.
- Three adversaries reject: missing import record, required touching donor omitted,
  and incorrect P1 observation. Invalid/inverted/nonfinite/over-budget requests
  are also covered by the full native unit matrix and preserve source storage.

`large_source_regression_v3/validation.json` PASS/stable pins: MPI1 has66 cases/
4508 assertions; MPI2/4 have21 distributed cases per rank. All1350 preceding
mesh/source JSON and SU2 artifacts are byte-identical to `recovery_integration_v2`;
`mesh_identity.json` retains every hash. Distributed sensor transport/residual,
full anisotropic/cycle/repair/recovery and their independent audits rerun.
`large_source_regression_v3/actual_nonmpi/validation.json` PASS verifies real non-MPI6->48->6->48 cycles,
movement/reconnection/recovery, exhaustion, input rejection preservation and
composed-edge refusal. The non-MPI runner also compiles/executes the direct import
and common distributed unit cases (8 cases/99 assertions); their fixture setup no longer depends on an
MPI-only helper. This test-only followup leaves the timed v2 production code unchanged.
No production 2D/MMG/BL/fade/noise path changed.

## Performance and rejected approaches

An unconditional cell filter was correct but unnecessarily costly for small
sources: matched serial coarsening median .3583s OFF/.3627s detail ON in
`large_source_regression_v1/actual_nonmpi`. That policy is rejected for cost;
its sources, logs, audits and pins are retained. `large_sources_v2` records its
correctness-passing larger-source pilot. `large_sources_v3` retains a terminal
truncated-record failure from attempted variable-width requests. The final fixed
record/conditional filter avoids both changes; shared 2D exchange is untouched.

`large_source_regression_v2/paired_checkpoint_timings/validation.json` PASS:
three alternating process pairs, same control source and verified binary pins.
Each process contains three OFF/ON trials; these are nine internal observations
per column, not nine independent process runs. Recorded accepted/rejected/edge
work matches. Coarsening median detail ON is .1941285s previous/.1951316s new;
ranges .1909460-.1978089 and .1940824-.2018688 overlap. OFF medians .1941502/
.1956198. Refine ON medians .0509431/.0489390; repair .0123374/.0116149. Do not
infer general speedup or overhead guarantees from these contended small controls.

Final source384 wedge parent maxima, including source/mesh migration, worker
communicator, engine and mesh return, are .12884s(N1/M1), .11639s(N2/M1),
.10806s(N2/M2), .10634s(N4/M2), .10057s(N4/M3), .10216s(N4/M4).
These are single pilots, not scaling repetitions. Never sum per-rank maxima or
nested costs as elapsed time. Source preparation is outside this frozen-fixture
parent timer; source indexing is inside engine initialization. Diagnostic JSON/
SU2 exports and independent audits are outside that event timer, separately
visible in runner stages. Actual partition/CFD lifecycle remains absent.

New counters expose donor candidates/filtered/payload/max patch, nested owner
search/filter time and donor admission bytes in the engine report. In the N1/M1
wedge, exact edge tracing and filtering dominate; inspect the raw closed phases
and `large_sources_v4/summary.json`. Maximum reported requested working storage
for the wedge is3,149,784bytes; source750 rejection reports its larger bound.
These are models, not per-rank peak RSS or measured MPI wait. Timings are recorded
under host contention; one owned job, build1/MPI<=4, nice19/library threads1.

## Inspect and replay

Actual grids and source/mesh JSON:
`large_sources_v4/mpi{1,2,4}_meshes/wedge_initial.su2`,
`wedge_m*.su2`, `coarse_initial.su2`, `coarse_m*.su2`,
`oversized_initial.su2`, `oversized_m*_rejected.su2`.
The rejected suffix labels a rejected import/event; it does not label invalid cells.

After a real host check, announce a NEW absolute case folder and run sequentially:
```bash
PYTHONDONTWRITEBYTECODE=1 nice -n19 python3   integration_evidence/native_3d_core_v1/run_large_sources.py /absolute/NEW_CASE
```
The runner refuses overwrite, pins source/binary/artifacts, captures failed
stages and launches MPI1/2/4 plus independent rational audits one at a time.
MPI/compiler/Eigen and the checked-out repository are required; no CFD binary.
The pinned old/new actual non-MPI control folders can be compared with
`run_checkpoint_timings.py BASELINE_CONTROL CURRENT_CONTROL NEW_FOLDER` after
those binaries have been rebuilt and their receipts verified.

## Remaining limits and next work

The source750 coarse-cell rejection is a real practical limit: complete-cell
imports still include too many original donors. Filtering does not make support
independent of original resolution. Point/segment query footprints or bounded
staged imports are a next candidate; increasing worker count does not remove the
per-caller union cap. The large-source filter is qualified here on unit boxes/
wedges; transformed larger-source cases and multiple/partial cavities remain to
be added. The earlier thin/sheared/rotated-metric matrix reruns with small sources.

Original coplanar finite triangle seams still constrain boundary motion/removal.
A geometry-patch authority must permit compatible surface retriangulation while
preserving finite patch coverage, features and markers. Cost-weighted tetrahedral
ParMETIS, production runtime/rebuild/solution/history return, native CGNS output,
full lifecycle/RSS/wait/imbalance and cluster scaling are unfinished. Production
3D stays guarded. No robust BL generation, curved geometry, unsteady3D or general
CFD affordability claim. Goal1 is not complete.
