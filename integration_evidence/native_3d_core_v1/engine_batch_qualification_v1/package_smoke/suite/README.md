The integrated candidate-batch milestone can use this same comparison harness with
`prepare.py --comparison-kind candidate_batch`. It requires the current engine
milestone PASS/source pins and freezes identical full-driver binaries. The two
controls set the test-only batch1/batch8 parameter, preserving strict targets.
No isolated operator is compiled for this mode. Run the existing serialized SGE
correctness gate and N1/2/4 timing jobs through `submit.py`. Size8 paired timings
retain the explicit1024diagnostic allowance for both controls so the baseline can
complete; separate local default512qualification belongs to the engine milestone.
Each batch/worker trajectory receives a fresh full audit, then three paired
repetitions, detailOFF/ON controls and optional mpiP. Compare total operational
cost, accepted edits per second and memory; fewer epochs alone is insufficient.
No cluster job is launched by preparation. Preserve compact first-trajectory
meshes/states and repeat fingerprints. Preparation is not timing/scaling evidence.

# Native 3D reservation-scheduler comparison on SGE

Repository: `/media/rausa/4TB/SU2_Versions/SU2_NativeIntegrated`.
Branch: **codex/native-3d-transport-accounting**. Production uses the standard
scheduler; rotating reservation priority remains an isolated experiment.
The previously qualified prototype passes the full97-case MPI1/2/4 regression,
stronger aspect2/H20 whole cases, actual CGNS restart and six real lifecycle
controls. See `integration_evidence/native_3d_core_v1/ROTATING_RESERVATION_WHOLE_VALIDATION.md`.
This campaign measures whether its better proposal balance actually lowers cost.
Preparation, orchestration checks and local correctness are not cluster results.

## What is compared

Use one up-to-date configured normal/MPI/MMG/CGNS/test build. The preparer freezes
the standard driver, then compiles only the qualified private directory variant
and links it before the same68test objects and unchanged archives. The normal
directory archive member must remain unextracted. Only the variant's include
location changes; its reservation implementation stays identical to the qualified
source. No production CPP, public configuration, target, acceptance or completion
policy changes. Old or unrelated compiled source versions reject before preparation.
Source/object/archive/binary/recipe pins and link maps are retained.

The candidate carries one checked priority-phase word per claimed vertex through
existing admission votes; it adds no collective. This changes scheduling, accepted
meshes/work and subsequent frozen original-P1 triangulations. **This is an
operational-cost comparison between independently audited trajectories, not a
fixed-work speed comparison.** Initial geometry and frozen target match exactly;
later continuous requested sensor recipes and q/J/L contracts match, but their
original-P1 interpolants can differ. Report achieved geometry/metric/state bounds,
final sizes, work rates and total costs together. Balanced opportunities alone
need not improve performance: the saved small restart case takes more rounds.

One N4 correctness job validates both binaries using their actual full regression
and independent audits. Serialized N1/N2/N4 comparison jobs then exercise M=N
and M=max(1,N/2), beginning with each binary's real Hessian/CFD/BDF controls.
For each N/M/scheduler: one warm-up and three paired repeats with detail OFF/ON;
scheduler/detail order alternates. Full independent exact audit on the first run
of each scheduler/N/M; later runs must match that scheduler's13raw geometry,
frozen-source, state and readback files exactly. Every actual SU2/CGNS output and
phase/counter record is checked fresh. Initial targets must match across schedulers.
Changed trajectories within one group or a failed strict q/J/L check stop the
campaign and retain all artifacts. No automatic budget relaxation or resubmission.

Default fixture is the turning/spatial size8 wedge with the existing explicit
1024-round diagnostic. Production512is unchanged. `--size4` selects the512-round
control. This experiment does not resolve the known larger default512failure.
Total parent adaptation wall includes metric construction, diagnostic observation,
remesh/partition/migration and return/rebuild/transfer. Exclusive phases close
against their parents; nested scopes and rank maxima are not added twice. Parent
max wall, accepted edits/sec and detail overhead are reported separately. Fixture
process wall also includes assertions/output; external audits/collection are
outside adaptation. Synthetic fields cannot establish physical-CFD affordability.

Keep ordinary query-prefilter fixed-work comparisons separate: their scripts and
bundles are unchanged under `native_cluster_3d_query_comparison_v1`.

## Prepare on the login host

Use your existing compiler/MPI/MMG/SCOTCH environment and actual configured build.
No second normal build is needed; prefix `$(pwd)` remains supported. Nodes need
no Git or compiler; the extra one-file compile/link happens during preparation.

```sh
cd /global-scratch/bulk_pool/arausa/SU2_Native
git checkout codex/native-3d-transport-accounting
git pull --ff-only
./ninja -C "$NATIVE3D_BUILD" -j2 UnitTests/test_driver
PYTHONDONTWRITEBYTECODE=1 python3 integration_evidence/native_cluster_3d_reservation_comparison_v1/prepare.py \
  --build-dir "$NATIVE3D_BUILD" --ninja ./ninja \
  --output integration_evidence/native_cluster_3d_reservation_comparison_v1/prepared_v1
```

`NATIVE3D_BUILD` must name your current normal/MPI/MMG/CGNS/test build, not bin.
Preparation keeps previous outputs; choose a fresh output if you already used
`prepared_v1`. Failures and their logs are retained. All runtime input files are
regular closed copies, including both binaries. Dynamic compiler/MPI/MMG/SCOTCH
libraries, Python numpy/h5py and the scheduler machinefile remain platform needs.
The disposable extra object is removed only after a successful link with its
hash retained. Preparation does not run MPI or submit jobs.

Optional paired MPI API profiling uses the same compatible library on both sides:
add `--mpip-library /path/to/libmpiP.so --mpip-license /path/to/LICENSE` during
preparation, and `--mpi-profile` during submission below. MPI API time is not
pure waiting or wire traffic; profiler overhead is distinct from detail overhead.

## Submit when ready

From the repository root, explicitly invoke:

```sh
PYTHONDONTWRITEBYTECODE=1 python3 integration_evidence/native_cluster_3d_reservation_comparison_v1/submit.py \
  integration_evidence/native_cluster_3d_reservation_comparison_v1/prepared_v1
```

Four jobs serialize through `hold_jid`: N4 correctness, then N1/N2/N4 comparison.
The timing jobs refuse a failed or mismatched correctness gate. The existing
`aero-ags.q`/`mpi`/GCC conventions are used; no exclusive node is assumed.
`--hold-jid JOBID` chains behind another campaign; `--size4` chooses the smaller
control. Optional `--mpi-profile` appends three serialized API-profile jobs.
Do not submit this simultaneously with the donor comparison. Nothing is submitted
by merely preparing or checking this tooling.

## Download only compact results

The submission receipt prints exact folders under `ClusterResults/native3d_reservation_*`.
Download those plus the receipt and launcher logs; no `ClusterRaw`, binaries or
build tree is needed for analysis. Successful repeats retain logs/config/environment,
phase/rank summaries and source/manifest/audit provenance. One full compressed
reference per scheduler/N/M retains inspectable meshes/current/BDF data and actual
outputs; matching duplicates are deleted only after verified equivalence and
fresh output checks. Failed runs retain everything. Source-owner donor counts,
requester payloads, private attempts/rejects/vetoes and active-worker loads stay
separate. Tracked transport omits vendor/direct MPI and includes self traffic.
Lifetime RSS and requested storage are not an allocator envelope.

```sh
PYTHONDONTWRITEBYTECODE=1 python3 integration_evidence/native_cluster_3d_reservation_comparison_v1/analyze.py \
  /absolute/path/to/ClusterResults/native3d_reservation_JOB1_timing_n1 \
  /absolute/path/to/ClusterResults/native3d_reservation_JOB2_timing_n2 \
  /absolute/path/to/ClusterResults/native3d_reservation_JOB4_timing_n4 \
  --output /absolute/path/to/ClusterResults/reservation_comparison_summary_v1.json
```

Analysis re-parses saved logs and verifies every recorded runtime value and
artifact hash. It requires three complete paired repetitions, constant work
within each scheduler and unchanged work between overhead controls; accepted
counts may differ across schedulers and are reported separately. Costs use
rank-max parent wall, with repetition spread and accepted-work rates. Per-rank
profiles retain partition/migration, query/reconstruction/commit, return/rebuild/
transfer, traffic/storage and reservation observations. Different N/M groups are
separate trajectories, not automatically fixed-work scaling. No promotion or
fullGoal1 completion follows without interpreting actual repeated results.
