# Native3D donor-query routing comparison on SGE

Repository: `/media/rausa/4TB/SU2_Versions/SU2_NativeIntegrated`.
Compile candidate branch **codex/native-3d-transport-accounting** after updating it.
Current candidate includes qualified grouped requests; see
`integration_evidence/native_3d_core_v1/QUERY_PACKING_VALIDATION.md`.
The earlier validated narrow routing checkpoint7b4e0d767fbda42ff17064ed93960316d794d5ab
remains a supported frozen candidate or a baseline for isolating packing.
Baseline is **2d3ac1e3a552a1b90a87dfa3595fc1b372783ceb**, including the full-star
facet fix and certified1024-round n8 capability, before narrow query routing.
Use matching compiler, MPI, external libraries and build options on both builds.
Preparation pins all four relevant source files and checks the complete expected
compiled source difference for each supported forward pair, plus equal build
options except installation prefix. Allowed pairs: original->routing, original->
packed and routing->packed; variants are recorded in comparison_manifest.json.
The old2D e6995fb baseline is not the baseline for this3D comparison.
For a packing-only comparison, use the same baseline preparation commands below
but checkout7b4e0d767fbda42ff17064ed93960316d794d5ab instead of2d3ac1e3a552a1b90a87dfa3595fc1b372783ceb.
No extra CLI option is needed: frozen source hashes identify the pair. Do not
compare elapsed time from a single local contended sample; that sample was slower
with packing despite fewer tracked bytes. Measure the complete cycle on cluster.
A fresh current candidate package uses the expanded97-case correctness selection
and seven distributed-record audits. Existing baseline/candidate frozen packages
retain their own earlier72/73-case recipes and declared evidence. Each side runs
its own frozen correctness tool. This harness expansion does not change the
declared compiled source pair or timed target/budget. Prepare a fresh
candidate bundle after pulling; do not edit already frozen package files.

No submission has been made. No local timing campaign is required. The package
uses the existing SGE queue/compiler/library conventions and no Git on compute
nodes. It reserves four slots for both-source correctness, then1/2/4slots for
serialized comparison jobs. Worker subsets are N=M and M=max(1,N/2). Each binary
also runs its actual Hessian/CFD/BDF lifecycle controls before measured fixtures.
Each N/M has one warm-up per binary and three paired repetitions of detailOFF/ON;
binary order and detail order alternate. Default fixture is size8, rotated/spatial
wedge, existing diagnostic budget1024, q>=.20,J>=.05,L<=1.8,noise0. Production
budget512 is unchanged. `--size4` gives the smaller512-round control.

The first baseline warm-up per N/M receives the full independent geometry/field/
BDF/output audit. Subsequent runs must match its complete frame/state/readback
hashes; actual SU2/CGNS files and current phase/rank accounting are checked fresh.
Changed trajectories stop the fixed-work comparison and retain all files for
independent investigation. That is not proof of a bad mesh. Each N/M reference
is separate; do not label different-partition trajectories fixed-work scaling.

Only one full reference per N/M is retained, with verified compressed JSON.
Matching repetitions retain configs, environment, stdout/stderr, execution stages,
rank/phase profiles and reference/hash receipts; duplicate grids/states are
removed only after equivalence and output checks. Failures keep all artifacts.
Downloaded `ClusterResults` includes code/source/manifest provenance without
binaries, allowing analysis without the original checkout or ClusterRaw folders.
External audits/compression are outside timed adaptation. Process wall includes
fixture assertions/output; parent wall includes diagnostic observer work. Nested
phases and per-rank maxima are never added to form elapsed time. Tracked transport
includes self traffic and omits vendor/direct MPI; CPU/elapsed is not pure wait.
The comparison does not establish physical-CFD affordability, complete MPI wire/
wait or allocator calibration, larger-rank limits or fullGoal1 completion.

Prepare the baseline in a separate checkout, leaving the candidate checkout intact.
With older Git, `git checkout` is sufficient:

```sh
cd /absolute/path/to/SU2_Native_QueryBaseline
git checkout 2d3ac1e3a552a1b90a87dfa3595fc1b372783ceb
git submodule update --init --recursive
NATIVE3D_MMG_ROOT=/home/aero/arausa/global/Libs/mmg-5.8.0/install
NATIVE3D_SCOTCH_ROOT=/global-scratch/bulk_pool/aero/utils_gcc10/SCOTCH_INSTALL
export LD_LIBRARY_PATH="$NATIVE3D_SCOTCH_ROOT/lib64:$NATIVE3D_MMG_ROOT/lib64:$NATIVE3D_MMG_ROOT/lib:${LD_LIBRARY_PATH:-}"
python3 meson.py setup build-query-baseline --prefix="$(pwd)" \
  -Denable-tests=true -Denable-normal=true -Denable-mmg=true \
  -Dmmg_root="$NATIVE3D_MMG_ROOT" -Dmmg_scotch_root="$NATIVE3D_SCOTCH_ROOT" \
  -Denable-cgns=true -Dwith-mpi=enabled
./ninja -C build-query-baseline -j2 UnitTests/test_driver
PYTHONDONTWRITEBYTECODE=1 python3 integration_evidence/native_cluster_3d_core_v1/prepare.py \
  --build-dir build-query-baseline --ninja ./ninja \
  --output integration_evidence/native_cluster_3d_core_v1/prepared_query_baseline
```

Substitute your actual baseline checkout. Existing configured baseline builds can
be reused when they match that source and options; do not create another full
build unnecessarily. Use `ninja` if that is your installed command. Both builds
must use the same source mode/options and compatible MPI/MMG/SCOTCH environment.
The core preparation rejects stale/disabled builds and keeps previous bundles.

From the candidate cluster checkout, after updating the branch:

```sh
cd /global-scratch/bulk_pool/arausa/SU2_Native
git checkout codex/native-3d-transport-accounting
git pull --ff-only
./ninja -C "$NATIVE3D_BUILD" -j2 UnitTests/test_driver
PYTHONDONTWRITEBYTECODE=1 python3 integration_evidence/native_cluster_3d_core_v1/prepare.py \
  --build-dir "$NATIVE3D_BUILD" --ninja ./ninja \
  --output integration_evidence/native_cluster_3d_core_v1/prepared_query_candidate
```

`NATIVE3D_BUILD` is your actual configured candidate normal/MPI/MMG/CGNS/test build.
Retain its working compiler/MMG/SCOTCH environment and `--prefix="$(pwd)"` if used.
Prepare the closed comparison, substituting the actual baseline package path:

```sh
SU2_QUERY_BASELINE_PACKAGE=/absolute/path/to/SU2_Native_QueryBaseline/integration_evidence/native_cluster_3d_core_v1/prepared_query_baseline
SU2_QUERY_CANDIDATE_PACKAGE="$PWD/integration_evidence/native_cluster_3d_core_v1/prepared_query_candidate"
PYTHONDONTWRITEBYTECODE=1 python3 integration_evidence/native_cluster_3d_query_comparison_v1/prepare.py \
  --baseline "$SU2_QUERY_BASELINE_PACKAGE" --candidate "$SU2_QUERY_CANDIDATE_PACKAGE" \
  --output integration_evidence/native_cluster_3d_query_comparison_v1/prepared_query_comparison
```

Preparation copies both binaries once plus their frozen recipes/provenance and
shared auditors into regular files; there are no external input links. Dynamic
MPI/MMG/SCOTCH and Python numpy/h5py remain platform requirements. Preparation
is not validation. Do not edit a frozen comparison; choose a new output if needed.

When ready to launch, from the candidate repository root:

```sh
PYTHONDONTWRITEBYTECODE=1 python3 integration_evidence/native_cluster_3d_query_comparison_v1/submit.py \
  integration_evidence/native_cluster_3d_query_comparison_v1/prepared_query_comparison
```

Use the repository submission script, not a script from a relocated bundle.
This queues four serialized jobs and retains a submission receipt. `--hold-jid 123456` waits for an already running job before the first submission. No exclusive
node is required; record and inspect run variability/host contention. The default
size8 is the hotspot comparison; use `--size4` for an additional smaller control.
Do not submit both simultaneously; serialize through the previous receipt's last
job. Jobs refuse a failed or different-source correctness gate. A later timing
failure does not authorize automatic resubmission or a relaxed numerical target.

Download the four named folders under `ClusterResults` plus the submission receipt.
To summarize successful timing folders after download:

```sh
PYTHONDONTWRITEBYTECODE=1 python3 integration_evidence/native_cluster_3d_query_comparison_v1/analyze.py \
  /absolute/path/to/ClusterResults/native3d_compare_JOB1_timing_n1 \
  /absolute/path/to/ClusterResults/native3d_compare_JOB2_timing_n2 \
  /absolute/path/to/ClusterResults/native3d_compare_JOB4_timing_n4 \
  --output /absolute/path/to/ClusterResults/query_comparison_summary_v1.json
```

The analyst verifies retained hashes, requires three paired repetitions and
constant accepted work, reports spread, candidate/baseline ratios, accepted edits
per second and detailON/OFF overhead. Detailed rank profiles retain query/import,
partition/migration, reconstruction, return/rebuild/transfer and memory measures.
Investigate extra request traffic and any changed M/N trajectory before declaring
that the serial pilot benefit survives MPI. A successful matrix is evidence for
its declared envelope, not a universal affordability or Goal1 certificate.

Optional MPI API profiling is separate from normal detail overhead. Prepare BOTH
core bundles with the same MPI-compatible `--mpip-library /path/to/libmpiP.so`
and `--mpip-license /path/to/LICENSE` using their existing core preparation tool,
then freeze a new comparison output. Append `--mpi-profile` to the repository
comparison submission command to append three serialized profiling jobs. Detail
is alwaysON in those jobs; profilerOFF/ON order alternates and each has three
paired repetitions. Reports cover the whole process, including assertions/output;
MPI API elapsed is not pure wait and its bytes are not physical wire traffic.
Inherited `LD_PRELOAD`/`MPIP` is rejected for these controls. The analyst keeps
normal detail overhead separate from profiler overhead and verifies the retained
full report identity. Runtime summaries are re-parsed from actual saved logs.
