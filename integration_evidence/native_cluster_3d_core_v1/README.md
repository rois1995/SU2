# Native 3D closed SGE validation and timing package

Working repository: `/media/rausa/4TB/SU2_Versions/SU2_NativeIntegrated`, branch
`codex/native-3d-core`. This is an initial MPI1/2/4 correctness, repeated timing
and instrumentation-overhead package. Larger sizes/ranks, full traffic/wait and
allocator calibration remain Goal1 work. The current reference-admission/query-accounting code builds and passes the serial
69-case native/reference/2D+3Dbridge suite and serialMMG checks. MPI2/4 and
independent fixed-work/restart qualification remain pending; package self-checks
are separate from those numerical checks.
Do not treat preparation or submission as a numerical PASS.

Use the cluster's existing GCC/MPI/MMG/CGNS build environment on the login host.
Build the complete normal-mode test driver with MPI, MMG, CGNS and tests enabled:

```sh
./meson.py configure build-native -Denable-tests=true -Denable-normal=true \
  -Denable-mmg=true -Denable-cgns=true -Dwith-mpi=enabled
ninja -C build-native UnitTests/test_driver
PYTHONDONTWRITEBYTECODE=1 python3 integration_evidence/native_cluster_3d_core_v1/prepare.py \
  --build-dir build-native --output integration_evidence/native_cluster_3d_core_v1/prepared_v1
```

`build-native` denotes your existing configured normal build directory; substitute
its actual name and retain your cluster compiler/MMG library options. If creating
a fresh directory, use `./meson.py setup build-native` with the same options
instead of `configure`. If Ninja is `./ninja`, use that for the build and pass
`--ninja ./ninja` to preparation. Use a new prepared output folder if `prepared_v1`
already exists; preparation deliberately preserves previous bundles. Preparation refuses a stale driver, missing MPI or disabled required build options before copying it,
pins all68 objects and actual archives/thin members, records sources/build options
and copies native3D implementation/test recipes and independent auditors.
The bundle has regular files only and contains its executable; it has no input
links outside itself. MPI and other dynamic system libraries remain platform
requirements. Prepare again in a new folder after any code or binary change.

When launch is authorized, from the repository root:

```sh
PYTHONDONTWRITEBYTECODE=1 python3 integration_evidence/native_cluster_3d_core_v1/submit.py \
  integration_evidence/native_cluster_3d_core_v1/prepared_v1
```

This submits seven serialized SGE jobs to `aero-ags.q`: correctness on four slots
(serialMMG, native/reference/2D+3Dbridge tests on MPI1/2/4 and actual persisted
SU2N1M1/CGNSN4M2 restarts), actual metric/Hessian lifecycle at N1/2/4, then
manufactured n4 wedge timing at N1/2/4. Each later job checks the preceding
correctness result; timing also checks its same-rank lifecycle result. All gates
require the identical frozen bundle. Partial submissions retain job IDs, commands
and the pending scheduler acknowledgement in `ClusterResults`. Inspect that
receipt/queue before retrying a failed submission; the script never resubmits
an existing campaign automatically.

Compute nodes need no Git or compiler. The SGE script uses `MACHINEFILE_PATH` or
`machinefile.$JOB_ID`, one numerical-library thread, and the frozen executable.
Each job runs one MPI launch at a time. Timing uses N=M and M=max(1,N/2), a warm-up
and three repetitions with alternating OFF/ON order. Each event remains a
three-event refine/coarsen/refine manufactured target; no extra CFD iterations
are introduced. Actual metric/Hessian/repartition/rebuild/transfer are measured
in the separate existing lifecycle fixture. Different N/M mesh trajectories are
independently audited and must not be described as fixed-work speedups.

Download only `ClusterResults/native3d_*` plus their
`ClusterResults/launcher/native3d_*` logs. These contain raw stdout/stderr, source
and build/binary pins, code/audit tools, strict audit records, inspectable SU2/CGNS
meshes, sidecars and BDF state files. No executable, object or build tree is
copied into results. Large mesh/source/state JSON files are gzip compressed only
after audit and verified exact decompression; failure artifacts remain raw.
Audits, compression and collection occur outside adaptation timers. Native packed
byte counters include self traffic and wrapper collective time is not pure wait;
the package does not repair those remaining profiling limits.

Review downloaded campaigns without running a solver or scheduler:

```sh
PYTHONDONTWRITEBYTECODE=1 python3 integration_evidence/native_cluster_3d_core_v1/analyze.py \
  ClusterResults/native3d_<job>_correctness_n4 \
  ClusterResults/native3d_<job>_lifecycle_n1 \
  ClusterResults/native3d_<job>_timing_n1
PYTHONDONTWRITEBYTECODE=1 python3 integration_evidence/native_cluster_3d_core_v1/restore.py \
  ClusterResults/native3d_<job>_timing_n1
```

Use all seven actual folders for the initial comparison. For the first larger
practical-limit campaign, append `--limits` to the submission command. This adds
one serialized four-slot job after timing, gated by same-bundle correctness and
the N4 lifecycle result. It runs n8 wedge N1M1, N2M2, N4M2 and N4M4 sequentially,
stopping at the first test/audit failure. The initial mesh has405 vertices,
1536 tetrahedra and512 boundary triangles, exceeding the former256-facet cap.
Dyadic coordinates keep the planar marker junction representable. The unchanged
target sequence is64/16/64, aspect1.5 with spatial rotation and variation0.125;
quality0.20, sliver0.05, length1.8,512 rounds and noise0 remain fixed.

These are single practical-limit pilots, not repeated timing comparisons.
`analyze.py` reports per-event parent elapsed time, useful accepted work, imbalance,
native bound and lifetime RSS for successful limits. It refuses failed campaigns;
inspect their `result.json`, failing case `parameters.json`,
`case_environment.json`, stages and raw logs/mesh/source/state files. Parameters
and the relevant test environment are saved before MPI launch, including on
failure. Failed-case files stay uncompressed. Failure stops later size/rank cases
inside that job; qsub hold ordering alone never counts as success. Do not rerun
with looser guards or more rounds to turn a practical limit into a PASS. The summary reports three
paired samples, medians/spread, accepted edits per parent-second, memory/RSS and
rank imbalance; exclusive and nested phase rows are labelled separately.
Analysis refuses an existing `native3d_summary.json`: preserve/rename the prior
summary before reanalysis. Restore keeps compressed originals and refuses changed
raw output. After restoration, existing `audit_envelope.py` or `audit_lifecycle.py`
can recheck the exact original evidence; `export_envelope_vtu.py` can generate
ParaView grids/states from an accepted envelope case.

Lightweight package checks, using fake files/subprocesses/mocked qsub only:

```sh
PYTHONDONTWRITEBYTECODE=1 python3 integration_evidence/native_cluster_3d_core_v1/check_package.py
```
