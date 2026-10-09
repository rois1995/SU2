# SGE native adaptation campaign

Compile `codex/native-unsteady-performance` from `rois1995/SU2`. The exact
built production sources are recorded in `source_pins.json`; rebuild both
executables after pulling C++ changes. For the matched old/new comparison, use
`../native_cluster_metric_comparison_v1/README.md`. In a checkout of the fork:

```bash
git fetch origin codex/native-unsteady-performance
git checkout -b codex/native-unsteady-performance origin/codex/native-unsteady-performance
git submodule update --init --recursive
```

Use a complete checkout with initialized build dependencies; the campaign and
inputs are tracked. Do not copy a worktree `.git` pointer without its object
database. The previously supplied bundle remains an optional offline checkpoint.

Build optimized primal-double executables with MPI and debug symbols, OpenMP off,
CGNS enabled and tests enabled, using the MPI/compiler installation that launches
jobs. For a fresh build directory, the native-only setup is:

```bash
CC=mpicc CXX=mpicxx python3 meson.py setup build-native --buildtype=debugoptimized \
  -Dwith-mpi=enabled -Dwith-omp=false -Denable-tests=true -Denable-cgns=true \
  -Denable-autodiff=false -Denable-directdiff=false \
  -Denable-mixedprec=false -Denable-singleprec=false -Denable-mmg=false
./ninja -C build-native -j2 SU2_CFD/src/SU2_CFD UnitTests/test_driver
```

Load the same GCC/MPI environment as your jobs before configuring. MMG is not
required for this native-only campaign. Build `SU2_CFD` and `test_driver`; use `build-native` as build directory or
set `SU2_CFD_BIN`/`SU2_TEST_BIN` to their paths inside the checkout. Keep the build
log/options and run the native MPI regressions before scaling. The campaign
checks all 762 built C++ source pins and input hashes before running; it
records binary hashes, linked libraries, compiler-host hardware and revision.
Git is optional on compute nodes: an unavailable revision is recorded as such;
source/input/binary hashes remain checked and the source-pin manifest hash is
recorded in job and case provenance. These checks do not by themselves prove
the binary was built from those sources.

[RunNativeSGE.sh](RunNativeSGE.sh) uses the supplied `aero-ags.q`/`mpi` environment
and GCC paths. Submit from the checkout root. It expects the existing scheduler
machinefile `machinefile.$JOB_ID`, exactly as the supplied script; if your site
places it elsewhere, set `MACHINEFILE_PATH`. The allocation is copied into the
repository before launches. No inputs or symlinks depend on workstation paths.
The compiler/MPI installation remains a cluster platform dependency.

Start with four ranks and two actual adaptations, then inspect the exported
independent mesh/metric/history gates:

```bash
qsub -pe mpi 4 -v BENCH_KIND=frozen_euler_to_bl,REPEATS=1,ADAP_WORKERS=4:3:2 integration_evidence/native_cluster_campaign_v1/RunNativeSGE.sh
qsub -pe mpi 4 -v BENCH_KIND=frozen_bl_to_euler,REPEATS=1,ADAP_WORKERS=4:3:2 integration_evidence/native_cluster_campaign_v1/RunNativeSGE.sh
qsub -pe mpi 4 -v BENCH_KIND=actual_euler_to_bl,REPEATS=1,ADAPT_EVENTS=2,ADAP_WORKERS=4:3:2 integration_evidence/native_cluster_campaign_v1/RunNativeSGE.sh
qsub -pe mpi 4 -v BENCH_KIND=actual_bl_to_euler,REPEATS=1,ADAPT_EVENTS=2,ADAP_WORKERS=4:3:2 integration_evidence/native_cluster_campaign_v1/RunNativeSGE.sh
```

Submit these pilots sequentially (or use SGE `-hold_jid` dependencies). Their
purpose is to check the build/MPI/data collection before a large matrix.
Each allocation includes the original N-worker partition as a control, then
weighted partitions with workers N, N/2 and N/4 sequentially, rotating order across
three repetitions. To choose workers explicitly, pass `ADAP_WORKERS=4:3:2`.
`actual_euler_to_bl` is SA RANS from the coarse Euler grid;
`actual_bl_to_euler` is Euler from the original BL grid, without a BL metric.
No `ADAP_HESSIAN_NOISE` filtering is enabled.

After successful independently audited pilots, set `REPEATS=3`, increase N
through 8,16,32 and request ten actual
adaptation events with `ADAPT_EVENTS=10`: RANS runs 2200 steps, Euler 1100.
Later try 64,96,192 only where the previous size merits it. This small RAE mesh
will expose an overdecomposition limit; it is not representative of a large
production mesh. Multi-node testing needs adequate work per rank. Jobs at
competing worker counts should use comparable node allocations and the
usual shared-node allocation, as requested. Serialize our jobs with SGE
dependencies; other users may still share the nodes, so timings are approximate.
Numerical threads=1. No broad scaling claim
follows from merely completing a high-rank run.

`ClusterRaw/<job>_<kind>/` retains every original output. Nothing is deleted.
`ClusterResults/cases/` contains only adapted/rejected meshes and references,
original donor snapshots at n/n-1, their transported restarts, final state,
configuration, complete solver logs, timing/metric CSVs and provenance.
`ClusterResults/jobs/` records allocation/hardware/binary/campaign summaries;
each `${JOB_ID}_launcher/` also retains the SGE wrapper stdout/stderr, source
revision and exit code, including failures before any solver case is created;
new exports retain matching audit code inside `ClusterResults/cases/CASE/tools/`,
with its hashes in that case's collection manifest. Earlier root-level
`ClusterResults/tools/` bundles remain unchanged. Different source versions can
coexist without overwriting earlier evidence. Download only `ClusterResults`,
then use each new case's own tools (Python + NumPy):

```bash
CASE_DIR=ClusterResults/cases/CASE
python3 "$CASE_DIR/tools/integration_evidence/audit_native_frozen_case.py" "$CASE_DIR" rae_euler_to_bl --self-contained
python3 "$CASE_DIR/tools/integration_evidence/audit_native_frozen_case.py" "$CASE_DIR" rae_bl_to_euler --self-contained
python3 "$CASE_DIR/tools/integration_evidence/audit_rae_unsteady.py" "$CASE_DIR"
python3 "$CASE_DIR/tools/integration_evidence/audit_native_profile_accounting.py" "$CASE_DIR"
```

Use the root-level bundle for historical exports that lack a case-local tools
folder, provided it matches the version used for those cases. Never replace an
older bundle with another version. The baseline checkout at e6995fbff5 keeps its
original export layout; this collection-only fix does not require rebuilding or
changing that baseline.

Choose the matching frozen command or the actual command, followed by profiling
accounting. Execution/collection PASS is not an independent mesh validation.
Collection reports missing files and preserves diagnostics even on solver failure.
Original path strings in provenance are historical records, not runtime links.

All actual controls output every step to retain both pre-transfer histories;
raw data stays on the cluster. This output policy adds CFD I/O cost and must be
reported when judging affordability. Transfer is nested in replacement; do not
add it twice. Compare full lifecycle and per-window costs, rank imbalance,
operations/cell counts and memory. Changed ranks/partitions can produce different
meshes: distinguish those trajectory comparisons from fixed-work speedups.
Host-local RSS samples are partial across nodes; frozen rank CSVs contain HWM
from every rank. CPU pressure/process samples do not replace site-wide contention
accounting. Heavy CPU/MPI profiling belongs to separate selected runs, with
optimized debug symbols; it is not enabled across the timing matrix.

The portable export audits cover fresh campaigns. Restart-case audits may still
need their parent checkpoint/reference and are not claimed to be portable by
this export recipe. Longer RAE histories may also invalidate the existing
near-constant FARFIELD assumption used to assess boundary-area changes; inspect
that diagnostic independently rather than relaxing mesh or transfer gates.
