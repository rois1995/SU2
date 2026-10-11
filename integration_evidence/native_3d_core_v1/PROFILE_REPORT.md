# Native 3D lifecycle profiling checkpoint

Repository `/media/rausa/4TB/SU2_Versions/SU2_NativeIntegrated`, branch
`codex/native-3d-core`, predecessor `d5df2785676e610b2dcaec2c5aee7ea93ceb9670`.
Full Goal 1 remains active and incomplete.

`WRT_PERFORMANCE=YES` now records per-rank exclusive wall/process-CPU scopes on
the native 3D path for sensors/Hessians/metric construction, CFD ParMETIS and
migration, geometry preprocessing, solver initialization, solution/BDF transfer,
publication/finalization and requested mesh/original-reference output. The
public AdaptMesh cycle encloses metric preparation, remeshing and replacement.
CFD partitioning after return from M workers is nested inside replacement time.
There are no new scheduling decisions, operators, targets or configuration knobs.
The 2D and MMG gates disable these new timers and profile collectives.

NATIVE3D_STAGE records close wall/CPU phases per rank. NATIVE3D_DETAIL records
expose the existing exclusive engine/import/partition phases and label nested
search/interpolation/composition, edge-tracing, gradation and donor costs. They
include requests/hits, retries, accepted/rejected work and private reconstruction/
metric/measurement CPU. Engine cache misses can be derived as requests minus hits;
bridge records contain explicit misses. Lifelong process peak RSS is separate
from modeled requested-storage bounds. CPU is never subtracted from wall time to
claim MPI waiting; process CPU includes MPI progress and any worker threads.

Each stage excludes its own record emission, while its parent includes nested
emissions. The outer testcase wall/CPU timer includes all cycle reporting and its
small computed-metric assertions/observation; requested output is a separate stage.
It is therefore a diagnostic fixed-work comparison, not production CFD timing.

## Qualification and preserved failures

`PROFILE_CHECKPOINT_VALIDATION.json` hashes terminal receipts and summaries.
profile_build_v1 retains the static-driver dimension compile error; the gate was
corrected to query the input geometry. profile_build_v2 passed the actual complete
production archives/executable and affected test objects. Final actual tests were
recompiled by profile_runtime_v2 after adding the test-only outer timer and fixing
the fixture's equal hmin/hmax, rejected by the existing strict configuration guard.
profile_runtime_v1 remains FAIL with that configuration error and earlier passing
runtime audits. No failure is rewritten or numerical acceptance gate relaxed.

profile_runtime_v2 PASS, all 42 stages, unchanged source/archive/object pins:

- Original actual driver matrix MPI1/2/4 and all M1..N:84 accepted events,
  42 exact detailed OFF/ON controls,42 SU2/CGNS output/actual reader roundtrips.
  All126 accepted/readback mesh signatures, original sensors and normalized
  transported current/BDF2 states match the published memory checkpoint.
- New actual metric-to-replacement fixture:36 accepted events, using three fresh
  inputs per N/M/detail with M1/2/4 <= N. The very narrow positive hmin/hmax interval
  makes a nearly4I constant target; the fixture saves the actual computed owner/
  halo tensors for strict independent audits. Scalar assertions check the bounded
  target, and actual current/BDF2 affine transfer is checked inside C++.
- 36 strict new mesh/source frames and their raw SU2/immutable sidecars pass
  independent geometry/embedding/incidence/marker/volume/metric audits. Thirty
  repeat/control comparisons preserve the exact computed original field and mesh.
  This fixture is manufactured and contains no CFD iteration or unsteady solve.
- 357 closed per-rank stage records and357 nested records, with parent/child
  containment checks; corrupted wall/CPU closure and NaN adversaries reject.
- MPI1/2/4 budget refusal/retry, default ownership/private-progress rejection,
  native2D driver and MPI-aware MMG2D/3D regressions pass. Raw default output audits
  pass too. Existing metric/shape/height/geometry contracts remain unchanged.

## Diagnostic timing evidence

`profile_runtime_v2/lifecycle_rank_phases.csv` contains all357 per-rank stage rows.
`lifecycle_phase_summary.json` retains18 ON cases, accepted work/s, stage max/mean
imbalance diagnostics and the three OFF/ON samples per fixed N/M. It never adds
rank maxima or calls sums of nested times elapsed time. To regenerate summaries:

    python3 integration_evidence/native_3d_core_v1/summarize_lifecycle.py /absolute/path/to/profile_runtime_v2

Outer-case medians (seconds), including test observation and cycle reporting,
excluding separately profiled requested mesh output:

| N / M | OFF median | ON median | ON/OFF |
|---|---:|---:|---:|
|1 / 1|0.068186|0.069705|1.022|
|2 / 1|0.083994|0.081335|0.968|
|2 / 2|0.080655|0.080980|1.004|
|4 / 1|0.102459|0.104808|1.023|
|4 / 2|0.112706|0.125878|1.117|
|4 / 4|0.228483|0.268899|1.177|

N4/M4 OFF samples span0.175267–0.466090s and ON0.177722–0.431321s. A foreign
four-rank FSI job was active; owned checks were sequential nice19/thread1 and
builds j1. This variability prevents attributing the distributed differences to
instrumentation or inferring useful scaling. Extra detailed-report collectives
can synchronize ranks; OFF controls and external timers must accompany future
campaigns. No speedup or affordability claim follows from these small fixtures.

For a concrete N1/M1 ON repeat1, exclusive remesher time62.5445ms consists of
support.0015, reference.0234, import1.2383, source.0542, weights.1855,
partition.0575, donor migration.0060, communicator.0663, refinement45.4803,
coarsening14.8313, wait.0116, return.5810 and residual.0077ms (rounded).
Refinement's nested engine scopes include metric17.4915ms and reconstruction
6.8027ms. The remaining engine scopes, including initialization/seed, are retained
in raw records; these two nested numbers do not sum to refinement time.
All new constant-target cases accept19 edits; ON parent accepted-edit rates range
with rank choice, approximately273/s at N1/M1 and71/s at N4/M4. These are tiny
same-input diagnostics; broader trajectories/accuracy and hardware differ.

## Inspectable cases and remaining requirements

The actual computed-sensor lifecycle mesh and source are in
`profile_runtime_v2/lifecycle_mpi4/lifecycle_m1_d1_r0.json`, corresponding
`lifecycle_m1_d1_r0_mesh_00001.su2` and original `.native_ref`, with its full CFG.
For independently audited transported sensor/current/BDF histories and CGNS,
use `profile_runtime_v2/mpi4/runtime_barycentric_m4_d0_s2_states.json`, mesh JSON,
`runtime_barycentric_m4_d0_mesh_00003.su2`/sidecar and
`runtime_barycentric_m4_d0_s2_written_CGNS.cgns`.

The reusable run_runtime.py now includes new budget and lifecycle checks and
independent audits, then the existing native2D/MMG/default regressions. All files
are repository-contained apart from the documented local configured build/tools.
Binary/object files remain ignored; compact logs/pins/meshes/states/failures remain.

Complete MPI wait/house-communication accounting and internal reader/ParMETIS
memory calibration are still missing. Scheduled WINDOW_AVERAGE/PREDICT driver
sampling is not fully decomposed by this steady/public-cycle profiler. Goal1 also
still needs practical larger/rotated/thin lifecycle and restart qualification,
size/rank limits and closed SGE campaigns. Only diagnostic serial/private CPU is
established here, without full unsteady3D, BL, curved-geometry or affordability
claims. Prepare cluster jobs without asking the unavailable user for launches.
