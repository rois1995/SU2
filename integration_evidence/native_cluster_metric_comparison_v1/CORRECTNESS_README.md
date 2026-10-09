# Cluster correctness gate for the new prediction history

Local MPI/CFD correctness checks were stopped at the user's request. The complete
primal-double MPI/CGNS build passed. The metric merge has its earlier completed
validation; the new selectable PREDICT history and native PREDICT support are
**not yet runtime validated**. Run this gate before the matched performance jobs.

From a complete checkout of `rois1995/SU2:codex/native-unsteady-performance`,
update and rebuild both executables on the login/build host using the native-only
MPI/CGNS configuration in [README.md](README.md). Rebuild after pulling; retain
compiler/MPI/options and build logs. Compute nodes need no Git. Python 3, NumPy
and h5py are required for independent SU2/CGNS output audits.

Submit from the repository root:

```bash
qsub -pe mpi 4 integration_evidence/native_cluster_metric_comparison_v1/RunPredictCorrectnessSGE.sh
```

It uses the same `aero-ags.q`, `mpi`, compiler environment and scheduler
`machinefile.$JOB_ID` convention as the existing campaign. Four allocated slots
are enough: MPI1, MPI2 and MPI4 checks and all solver cases run sequentially.
There is no workstation CPU mask, background ensemble, or exclusive-node request.
If another owned job is queued/running, add `-hold_jid JOB_ID` to serialize them.
This is a correctness job, not a performance measurement. Per-stage timeout is
two hours, and failures stop subsequent stages while preserving available output.

The job checks source, test, suite and input hashes before launching. Binaries
must resolve inside the checkout; a source hash alone cannot establish how they
were built. It records binary hashes and copies the scheduler allocation inside
the result folder. Complete inputs are regular tracked files, with no runtime
links into workstation cases or another checkout.

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
