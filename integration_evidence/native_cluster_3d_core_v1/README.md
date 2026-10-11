# Native 3D closed SGE validation and timing package

Working repository: `/media/rausa/4TB/SU2_Versions/SU2_NativeIntegrated`, branch
`codex/native-3d-transport-accounting`. This is the updated MPI1/2/4 correctness, repeated timing
and instrumentation-overhead package. Larger sizes/ranks, full traffic/wait and
allocator calibration remain Goal1 work. The current reference-admission/query/transport-accounting code builds and passes the serial
69-case native/reference/2D+3Dbridge suite and serialMMG checks. The 69-case
selection and independent bridge audit now pass on MPI1/2/4. The n4 frozen-work N1M1/N4M2 and persisted SU2/CGNS restart checks also pass locally.
Repeated cluster performance remains pending. The first local n8 N1M1 pilot
now rejects at the fixed coarsening attempt budget; see the practical-limit
section below. Package self-checks are separate from numerical checks.
Do not treat preparation or submission as a numerical PASS.

Use the cluster's existing GCC/MPI/MMG/CGNS build environment on the login host.
Earlier downloaded cluster checkpoints had enable-mmg=false. This suite also
checks the retained serial MMG2D/3D path and therefore needs its library/header
installation. The user-provided cluster prefix is
/home/aero/arausa/global/Libs/mmg-5.8.0/install. Set mmg_root explicitly and retain
your compiler/MPI environment. The prefix must contain include/mmg/libmmg.h and
libmmg.so or libmmg.a under lib or lib64; both directories are searched. MMG
executables alone are insufficient. The cluster prefix has lib64/libmmg.a, so
retain the prefix above rather than appending lib64 to mmg_root. For a shared
installation include its actual library directory in LD_LIBRARY_PATH before
submission; LD_LIBRARY_PATH does not repair static-library discovery.
The cluster static MMG archive references SCOTCH. The user confirmed the working
MMG executable loads libscotch.so.7.0 and libscotcherrexit.so.7.0 from
/global-scratch/bulk_pool/aero/utils_gcc10/SCOTCH_INSTALL/lib64. Supply the
SCOTCH_INSTALL prefix with -Dmmg_scotch_root and retain lib64 in LD_LIBRARY_PATH
on the login and compute nodes. This path observation does not itself verify
the SU2 cluster compile/link or runtime. For another installation, substitute
the SCOTCH prefix used to build its MMG library.

Build the complete normal-mode test driver with MPI, MMG, CGNS and tests enabled:

```sh
NATIVE3D_MMG_ROOT=/home/aero/arausa/global/Libs/mmg-5.8.0/install
NATIVE3D_SCOTCH_ROOT=/global-scratch/bulk_pool/aero/utils_gcc10/SCOTCH_INSTALL
export LD_LIBRARY_PATH="$NATIVE3D_SCOTCH_ROOT/lib64:$NATIVE3D_MMG_ROOT/lib64:$NATIVE3D_MMG_ROOT/lib:${LD_LIBRARY_PATH:-}"
./meson.py configure build-native -Denable-tests=true -Denable-normal=true \
  -Denable-mmg=true -Denable-cgns=true -Dwith-mpi=enabled \
  -Dmmg_root="$NATIVE3D_MMG_ROOT" -Dmmg_scotch_root="$NATIVE3D_SCOTCH_ROOT"
ninja -C build-native -j2 UnitTests/test_driver
PYTHONDONTWRITEBYTECODE=1 python3 integration_evidence/native_cluster_3d_core_v1/prepare.py \
  --build-dir build-native --output integration_evidence/native_cluster_3d_core_v1/prepared_v1
```

`build-native` denotes your existing configured normal build directory; substitute
its actual name and retain your cluster compiler/MMG library options. If creating
a fresh directory, use `./meson.py setup build-native` with the same options
instead of `configure`. If configure fails with KeyError for options, the saved
meson-private/cmd_line.txt is missing its options section. Keep the old build
intact and use setup with a fresh build-directory name, retaining your compiler/
MPI environment and additional options; pass that name to --build-dir below.
You can retain `--prefix="$(pwd)"`: it controls the
installation destination, while this package copies the test driver directly from
the build directory and requires no installation or local `/tmp` files.
If Ninja is `./ninja`, use that for the build and pass
`--ninja ./ninja` to preparation. Use a new prepared output folder if `prepared_v1`
already exists; preparation deliberately preserves previous bundles. Preparation refuses a stale driver, missing MPI or disabled required build options before copying it,
pins all68 objects and actual archives/thin members, records sources/build options
and copies native3D implementation/test recipes and independent auditors.
The bundle has regular files only and contains its executable; it has no input
links outside itself. MPI and other dynamic system libraries remain platform
requirements. Prepare again in a new folder after any code or binary change.

If compilation stops with `Disk quota exceeded`/`Quota disco superata` while
writing `.o.d` or `.o`, this is a filesystem write failure, separate from compiler
warnings and the earlier MMG/SCOTCH link failure. Preserve that build and its log;
creating another full build wastes space and does not fix quota enforcement.
The user reported this on2026-10-11 at21:47:56CEST during step672/926: user
252.88/300GiB and aero group5.15/10TiB in Default, with unlimited chunk-file
quotas. Checkout and build directories both assign new files to Default.
The aero flash pool is799.87/800GiB, but is not established as the cause of these
Default writes. Target capacity/inodes, underlying target quotas and stale
quota-enforcement state need separate checks. The `.o` and `.o.d` paths localize
this failure to the build; redirecting TMPDIR alone does not move those outputs.

Read-only checks from the cluster checkout, using the actual failed build name:

```sh
NATIVE3D_FAILED_BUILD=build-native-3d-20261011_212000
beegfs-ctl --getentryinfo "$NATIVE3D_FAILED_BUILD/SU2_CFD/src/libSU2core.a.p"
beegfs-df
beegfs-ctl --listtargets --longnodes --state
```

If a partial failed object remains, inspect it with --getentryinfo as well: file
placement need not match the current directory default. Give administrators the
failing paths,21:47:56CEST timestamp, UID1050/GID520 and the simultaneous quota
output. Ask them to check the relevant storage targets' cached exceeded-quota
state, backend block/inode capacity and underlying user/group limits. BeeGFS7
[quota enforcement](https://doc.beegfs.io/7.4.7/advanced_topics/quota.html)
refreshes periodically; its
[write handler](https://github.com/ThinkParQ/beegfs/blob/7.4.7/storage/source/net/message/session/rw/WriteLocalFileMsgEx.cpp)
checks a target-specific cached exceeded-quota store. The installed version and
server state remain unverified. After storage access recovers, resume Ninja in
the same build directory; package preparation still requires a completed build.

When launch is authorized, from the repository root:

```sh
PYTHONDONTWRITEBYTECODE=1 python3 integration_evidence/native_cluster_3d_core_v1/submit.py \
  integration_evidence/native_cluster_3d_core_v1/prepared_v1
```

This submits seven serialized SGE jobs to `aero-ags.q`: correctness on four slots
(serialMMG, native/reference/2D+3Dbridge tests on MPI1/2/4, independent bridge
geometry/markers/original-P1/current+BDF/SU2+CGNS audits at each rank count, and
actual persisted SU2N1M1/CGNSN4M2 restarts), actual metric/Hessian lifecycle at N1/2/4, then
manufactured n4 wedge timing at N1/2/4. Each later job checks the preceding
correctness result; timing also checks its same-rank lifecycle result. All gates
require the identical frozen bundle. Partial submissions retain job IDs, commands
and the pending scheduler acknowledgement in `ClusterResults`. Inspect that
receipt/queue before retrying a failed submission; the script never resubmits
an existing campaign automatically.

Submission exports only PATH and, when defined, LD_LIBRARY_PATH by name through
qsub -v. This preserves login MPI/Python and external shared-library search paths
without exporting the full environment. The SGE script prepends its declared GCC
paths while retaining those inherited paths. A bundle prepared before this fix
retains its old submit.py: prepare a fresh output after updating when a corrected
submission recipe is needed.

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

The local n8 N1M1 pilot and diagnostic rerun reject in cycle0: refinement uses
457 accepted edits, then coarsening reaches512 attempts (272 accepted/240
rejected) with91 candidates still queued. The private cache covers2489 cells
and records zero invalid cells, but completion requires the queue to be empty.
Evidence: integration_evidence/native_3d_core_v1/termination_limit_review_v1;
original FAIL folders transport_limit_n8_n1_m1_v1/v2 retain input grids/logs.
No accepted/rejected private grid or later cycle is available. The --limits job
will stop at this first case if the failure reproduces on the cluster.
New detailed engine rows report final_owned_cells, cached_measured_cells,
cached_invalid_cells, remaining_edges and remaining_repairs. These are local
termination diagnostics, not an independent mesh audit. Older logs without them
remain readable but cannot certify termination state. Acceptance is unchanged.

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

Offline summaries now include `rank_profiles`: closed phase fractions on the
rank with the largest parent wall time, separate active-worker breakdowns for
M<N, private CPU/work/query imbalance over active engine ranks, nested query/
partition/engine costs and lifecycle OFF/ON samples. Each phase's rank maximum
is a diagnostic; never sum those maxima or add nested detail to a parent.
Event indices are ordinals within each stage's log stream, not shared cycle IDs.
A worker count absent from a CFD stage is unknown. Process CPU includes MPI
progress; elapsed-minus-CPU is not waiting time. Full MPI traffic/wait and memory
calibration still remain open.

This is offline analysis only: if you already prepared or submitted the previous
checkpoint, keep that bundle unchanged and run the updated checkout's analyzer
when results arrive. No executable rebuild or new campaign is required for this
analysis change alone. Different frozen bundle identities still cannot be merged
into one paired campaign. Checked scope/evidence: PROFILE_ANALYSIS_REPORT.md.

### Optional deep MPI profiling (isolated transport branch)

This addition is on codex/native-3d-transport-accounting, not the currently
published codex/native-3d-core campaign. Keep existing prepared/submitted
bundles unchanged. Its orchestration has lightweight mocked checks; actual mpiP
loading, numerical qualification and profiling overhead are still unverified.

A shared mpiP library built with the same MPI as the test driver can be frozen
with its complete matching license/notices. Its runtime dependencies, like the
existing MPI/MMG/CGNS dependencies, must be installed on the compute nodes.
mpiP is optional and never needed by normal correctness/lifecycle/timing runs.
Do not use a library built for another MPI implementation.

For a future approved campaign, prepare a new bundle on the login host:

    python3 integration_evidence/native_cluster_3d_core_v1/prepare.py --build-dir build-native --output integration_evidence/native_cluster_3d_core_v1/prepared_mpip_v1 --mpip-library /absolute/path/to/libmpiP.so --mpip-license /absolute/path/to/matching-complete-license-and-notices.txt
    python3 integration_evidence/native_cluster_3d_core_v1/submit.py integration_evidence/native_cluster_3d_core_v1/prepared_mpip_v1 --mpi-profile

The flag appends three serialized N1/2/4 profiling jobs after the ordinary
seven-job campaign, with same-package correctness and matching lifecycle PASS
gates. M=N and M<N choices use the existing frozen n4 targets/guards. Each choice
has a warm-up and three alternating profiler OFF/ON pairs, with native detail ON
in both controls. Every mesh/source/current+BDF/readback hash must match.
Inherited LD_PRELOAD/MPIP are refused for this comparison. Preloading applies
only to the MPI application; neither mpirun nor auditors load the profiler.
An absent/incomplete/all-ranks-mismatched report fails the campaign and keeps
raw files uncompressed. The library is copied and pinned inside the bundle;
downloaded results retain its identity/license and reports but omit binaries.

The [mpiP documentation](https://software.llnl.gov/mpiP/) defines callsite
statistics and communicator/message-size histograms. Full .mpiP reports retain
per-rank data with four-frame callsites, collective and point-to-point histograms.
analyze.py reports paired adaptation-parent overhead and whole-process overhead;
the latter also includes the test fixture, output and final report generation.
MPI API elapsed is not pure wait or physical wire traffic. Statistical callsites
do not establish chronological phase windows, exact communicator identity,
allocator peaks or complete coverage of a particular MPI/vendor build. Inspect
the actual reports and calibrate before using them to identify a bottleneck.
Source lines require usable debug symbols. No time/speedup claim follows from
the mocked checks or the serial coverage lower bound.

The analyzer now includes mpi_api_profiles for every profiler-ON case (warm-up
explicitly marked). It uses the complete verbose callsite table, checks declared
stack/rank-record counts, exact per-callsite aggregate counts and per-rank time
closure within accumulated printed precision. API counts exclude aggregate rows
to avoid double counting. Per-function and callsite seconds are explicitly rank
work, not wall time. Callsite stacks remain attached; four-rank reports may
legitimately contain a callsite on only the two adaptation workers. Nonfinite,
negative, duplicate and incomplete records fail. The launcher fixes the print
threshold to zero; partial or concise reports cannot qualify these checks.

Format reference: [upstream report.c at 8a2ae9e](https://github.com/LLNL/mpiP/blob/8a2ae9e9458af16f6271b3280b1d5dbe4f121388/report.c).
The format source hash and a clearly synthetic four-rank report/summary are
retained in package_checks_v17. They validate parsing, not actual profiler
loading, MPI/vendor coverage, wall-clock overhead or a remeshing speedup.
