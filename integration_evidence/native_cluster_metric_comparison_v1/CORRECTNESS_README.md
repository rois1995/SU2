# Reusable native adaptation correctness gate

Use this gate after implementation changes. Heavy correctness and performance
checks belong on the cluster; local work is limited to source/script checks and
small fake-file harness tests. The native 2D PREDICT history/filter gate
passes completely in user-run cluster job 581856. A successful build or prepared
checkpoint does not replace that PASS. Run this gate before performance jobs.

The first cluster job (581847) passed 160/161 serial metric tests. Its sole failure
was an orientation mismatch in the new independent reference fixture: the mesh
generator deliberately emits clockwise triangles, whereas SU2 corrects them.
That fixture is now corrected without removing the exact connectivity or
predicted-field checks. Rebuild `test_driver` and rerun; later MPI/CFD stages
were not reached. The saved failure and connectivity replay are preserved in
`../native_post_rebase_metric_v1/cluster_correctness_581847_failure.json`.

The rerun (581852) passes all metric/native suites on MPI1/2/4 and all five
rejection guards. The first two native PREDICT CFD cases pass independent audits.
The third case stops at step 11 because its omitted MAX_TIME defaults to 1s;
15 steps at 0.1s need a larger time limit. All seven fixtures now explicitly set
MAX_TIME=100.0 so TIME_ITER controls completion. Its two completed remeshes also
pass a separately scoped saved-output audit, but the aggregate remains FAIL.
Later MPI CFD/CGNS/RANS/partial-restart cases are still pending. Preserve the
original results; pull and prepare a new checkpoint before rerunning the same
job. This fixture-only correction needs no production rebuild (ninja may be a
no-op). Evidence: `../native_post_rebase_metric_v1/cluster_correctness_581852/`.

The next rerun (581856) completes all 20 stages. All metric/native MPI1/2/4
suites, five guards, six Euler/CGNS/straight-wall SA RANS cases and three partial
restarts PASS and have been reviewed from saved outputs. Independent audits and
supplemental topology/marker/free-boundary geometry checks pass on all 11 mesh
replacements. The RANS composed-field nodal gradation diagnostic still reaches
ratio 2.73259 with 76 residual directed edges; sensor gradation alone passes.
This is a recorded limitation, not a combined-field certificate or performance
result. Reviewed evidence: `../native_post_rebase_metric_v1/cluster_correctness_581856/`.
Proceed to the separate serialized old/new pilot in README.md for performance;
repeat this correctness gate after future implementation changes.

From a complete checkout of `rois1995/SU2:codex/native-unsteady-performance`,
update and rebuild both executables on the login/build host using the native-only
MPI/CGNS configuration in [README.md](README.md). Rebuild after pulling; retain
compiler/MPI/options and build logs. Compute nodes need no Git. Python 3, NumPy
and h5py are required for independent SU2/CGNS output audits.

From the repository root, repeat this sequence after pulling or changing code:

```bash
./ninja -C build-native -j2 SU2_CFD/src/SU2_CFD UnitTests/test_driver
python3 integration_evidence/native_cluster_metric_comparison_v1/prepare_correctness.py
qsub -pe mpi 4 integration_evidence/native_cluster_metric_comparison_v1/RunPredictCorrectnessSGE.sh
```

It uses the same `aero-ags.q`, `mpi`, compiler environment and scheduler
`machinefile.$JOB_ID` convention as the existing campaign. Four allocated slots
are enough: MPI1, MPI2 and MPI4 checks and all solver cases run sequentially.
There is no workstation CPU mask, background ensemble, or exclusive-node request.
If another owned job is queued/running, add `-hold_jid JOB_ID` to serialize them.
This is a correctness job, not a performance measurement. Per-stage timeout is
two hours, and failures stop subsequent stages while preserving available output.

Preparation writes `build-native/native_correctness_checkpoint.json` with
`PREPARED_NOT_VALIDATED`, current production/test sources, suite/input hashes,
binary hashes and available Meson build-option metadata. It discovers new C++
source/test files and additional declared fixtures; intentional implementation
changes do not require editing the performance campaign's frozen source pins.
Reprepare after rebuilding changed code. Repeated checks of the same unchanged
build can reuse its checkpoint.

The job verifies the checkpoint before launching, guards source/suite files
between stages, and verifies sources and both binaries again before PASS. It
saves `checkpoint.json` with each job's results. A source snapshot alone cannot
prove binary build provenance: rebuild both executables first and retain build
logs. Never modify the checkout, binaries or checkpoint while its jobs are queued
or running; use separate complete checkouts for different versions. Earlier job
folders are preserved when a new checkpoint is prepared.

Binaries and suite files must resolve inside the checkout. The scheduler allocation
is copied into the result folder. Inputs are regular tracked files, with no runtime
links into workstation cases or another checkout. For alternative build paths,
use the preparation helper's `--binary`, `--test-binary` and `--output` options,
and pass matching `SU2_CFD_BIN`, `SU2_TEST_BIN` and `CORRECTNESS_CHECKPOINT` values
through `qsub -v`; the job rejects binaries that differ from the checkpoint.

Checks include:

- Metric and native regressions on MPI1/2/4, including manufactured feature
  acceleration, temporal jitter damping, 2D/3D accelerating metric transport,
  and gather/scatter agreement against a complete mesh.
- Rejection of invalid snapshot counts, excessive cadence spans, negative
  filtering, and the retained unsupported native FIXED_POINT guard.
- Native default two-snapshot Euler and four-snapshot Euler on N1/M1, N2/M1,
  N4/M3, plus CGNS output and a straight-wall SA RANS Euler-to-BL case.
- Independent original-connectivity metric quality/edge-length, geometry,
  prescribed height, flow positivity and both BDF-history conservation checks
  at every actual replacement.
- Actual adapted-mesh restarts at steps 7/8/9 to check three-/two-/one-snapshot
  fallback, unchanged cadence, matching final state and positive-definite metric.
  These short partial restarts contain no new remesh or history transfer.

MMG compatibility controls and the archived workstation old-output byte
comparison are omitted from this native-only job. The existing two-snapshot
unit regression checks unchanged MotionField arithmetic. This is not a native
3D remeshing, native fixed-point, converged aerodynamic or speed certificate.

Download only:

```text
ClusterResults/predict_correctness_JOB_ID/
ClusterResults/jobs/JOB_ID_predict_launcher/
```

The correctness folder contains all small-case grids, flow/restart snapshots,
geometry sidecars, configurations, logs, audit JSONs, and matching portable audit
tools. `validation.json` must report `status: PASS`; launcher exit code zero alone
is insufficient. Original-reference preservation and both-history conservation
are explicit gates, not merely solver success. Send this folder back for review.
Existing downloaded ClusterResults and raw evidence are never overwritten.

Full-window Euler/RANS cases can also be re-audited from the downloaded tools:

```bash
python3 ClusterResults/predict_correctness_JOB_ID/tools/integration_evidence/audit_native_unsteady.py ClusterResults/predict_correctness_JOB_ID/four_cgns_n4m3
```

Partial-window cases use the saved dedicated audit, because the full-window
checker deliberately requires a window-boundary restart. Once the correctness
job passes and is reviewed, follow [README.md](README.md) for the serialized
old/new metric performance comparison. That workload uses WINDOW_AVERAGE;
it does not measure the additional cost of more prediction snapshots.
