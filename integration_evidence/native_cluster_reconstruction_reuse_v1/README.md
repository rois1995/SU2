# Native reconstruction score and metric reuse comparison (SGE)

Compile **`codex/native-unsteady-performance`** in your existing cluster checkout.
This tests two performance changes against the validated profiled executable from
job **582199**: exact ordered-coordinate reuse of edge/cell scores and second-chance
retention of reused metric samples. No target, geometry, first-height, candidate
search order or acceptance threshold is intentionally changed. Sensor-only donor
interpolation and geometric BL composition at the actual query stay intact.

## Preserve the baseline, then build

Run from the complete checkout root on the login/build host, BEFORE rebuilding:

```bash
git checkout codex/native-unsteady-performance
git pull --ff-only origin codex/native-unsteady-performance
python3 integration_evidence/native_cluster_reconstruction_reuse_v1/prepare.py --preserve-control
./ninja -C build-native -j2 SU2_CFD/src/SU2_CFD UnitTests/test_driver
python3 integration_evidence/native_cluster_metric_comparison_v1/prepare_correctness.py
python3 integration_evidence/native_cluster_reconstruction_reuse_v1/prepare.py
```

Keep the build log and reuse your MPI/GCC10.2/CGNS/Meson settings. The new
preserved baseline is `build-native/reconstruction-control/test_driver`.
Its required SHA256 is
`1a092865535502a3cf42a4f4d16c481c3cdc45dc6feff1a9844afa22c181634f`.
A differing binary is refused; recover the original job582199 executable if it
has already been replaced. Rebuilding an old revision is not guaranteed to
reproduce that exact binary. Existing `build-native/balance-control` and historical
checkpoints/evidence are untouched. Source identities are preserved in
`control_source_pins.json`; executable/source pinning alone is not build provenance.

The new checkpoint is `build-native/reconstruction_reuse_checkpoint.json`.
Existing checkpoints are never overwritten: preserve/rename one before preparing
another build. Preparation declares PREPARED_NOT_VALIDATED. Sources, tests, both
packages, audit tools, fixtures and executable identities are verified before
cases and again before PASS. Do not rebuild/edit this checkout while queued/running.
All launch dependencies resolve inside the checkout; compute nodes need no Git.

## Run correctness first

```bash
qsub -pe mpi 4 integration_evidence/native_cluster_metric_comparison_v1/RunPredictCorrectnessSGE.sh
```

This reusable 20-stage gate now also selects the new score/cache regressions on
MPI1/2/4. It includes actual unsteady Euler/SA RANS, CGNS, geometry/height,
transported flow/BDF histories, prediction and restart checks. It remains the
small-case lifecycle check, not a new converged RAE simulation. See the existing
[correctness instructions](../native_cluster_metric_comparison_v1/CORRECTNESS_README.md).

Wait for its `ClusterResults/predict_correctness_JOB_ID/validation.json` to report
**PASS**, then run the comparison. Submit one owned job at a time; a scheduler
hold alone does not establish the previous job passed.

## Run the paired frozen comparison

```bash
qsub integration_evidence/native_cluster_reconstruction_reuse_v1/RunReconstructionReuseSGE.sh
```

One ordinary `aero-ags.q`/`mpi` shared-node allocation uses four slots. All cases
are sequential; no exclusive reservation, local solver run or submission by
Codex. Every actual working/retained testcase folder is printed. The first pilot
runs focused native/cache/profile units on MPI1/2/4, then **16 frozen remeshes**:
Euler-to-BL and BL-to-Euler, with paired baseline/candidate cases at N4/M4 NO,
N4/M4 YES, N4/M3 YES and N4/M2 YES. These are the same closed RAE fixtures/raw
sensors as job582199. No new CFD timesteps or Hessian computation occurs here.

Both roles enable the same bounded balance profiler. Existing role `control`
means job582199; role `profile` means the new candidate. Each pair must pass the
independent geometry/topology/metric/first-height and timing-accounting audits,
produce **byte-identical accepted mesh and all four transported-tensor CSVs**,
and retain identical per-rank operation/selection counts. A difference stops
subsequent pairs and must be investigated before timing interpretation.

`validation.json` records full remesh timings (including working repartition),
all existing exclusive scopes, rank/operation private wall and process-CPU costs,
requests/evaluations/evictions, longest attempt and bounded hotspots. Paired
summaries expose sampling/count and cost changes explicitly. Evaluations mean
cache misses, not distinct coordinates or Hessian recalculations. Maxima and
nested scopes must not be added. CPU coverage and shared-node observations remain
available in detailed records; rank modes still do different work and are not a
fixed-work strong-scaling curve. No per-query timers are added.

After reviewing one repetition, an optional separate repetition job is:

```bash
qsub -v REPEATS=3 integration_evidence/native_cluster_reconstruction_reuse_v1/RunReconstructionReuseSGE.sh
```

Pair/workload order reverses and partition order rotates across repeats. Shared
node contention means small gains need repeated evidence. This comparison tests
the combined improvements; it does not attribute speedup independently to each.

## Download only compact results

For correctness, download the existing `predict_correctness_JOB_ID` folder and
`ClusterResults/jobs/JOB_ID_predict_launcher`. For the new frozen comparison:

```text
ClusterResults/reconstruction_reuse_JOB_ID/
ClusterResults/jobs/JOB_ID_reconstruction_reuse_launcher/
```

Inspectable adapted meshes/tensors are in `cases/frozen_*_n4_m*_p*_r*_control/`
and the corresponding `_profile/`. The audited v1 runner is reused, including
closed exports, verified hardlinks or exclusive-copy fallback, bounded profiles,
audit tools saved once per job, selected logs/configs/timing/mesh/tensor evidence
and removal of new temporary work only after verified export. No ClusterRaw or
CFD timestep-output series is produced by the frozen job. Previous results remain
untouched. Use `rsync -aH` if hardlinks are available; otherwise copies are retained.

The score cache has 1024 fixed slots (about64KiB plus bounded star indices), scoped
to one immutable target/search. Changed ordered coordinates invalidate reuse;
collisions recompute. Scratch storage is included in dependency admission, so
unusually tight memory budgets can reject earlier. Metric storage stays capped at
2048 entries, including pinned authoritative vertex samples. A second-chance
policy cannot prevent eviction when all entries are hot. No claimed speedup,
large-core scaling, native3D or composed-BL-gradation certificate is made before
cluster evidence. Converged aerodynamics are outside this change.

Local preparation checks use fake files only:

```bash
PYTHONDONTWRITEBYTECODE=1 python3 integration_evidence/native_cluster_reconstruction_reuse_v1/check_package.py
```
