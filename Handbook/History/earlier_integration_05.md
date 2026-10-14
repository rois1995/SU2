## Pending integration and new PREDICT request (2026-10-08)

Working branch codex/native-post-rebase-metric-candidate has an uncommitted merge
of metric head6285044614 onto d8fc473a00. Production auto-merge is clean; the RAE
CSV/Euler/transport audit conflict is resolved using the immutable original wall.
Combined executable archive native_post_rebase_metric_v1/build_v2 is pinned.
MPI1 serial metric163/native87 cases and MPI2 metric38/native87 per rank PASS.
MPI4 metric37 and steady metric reuse separately PASS, but GoalMPI followed by
SteadyMetricReuse reproducibly times out in native BULK_FLIP (round45, 14 commits).
CustomSensorsMPI->steady and recovery->steady PASS. Do not publish as fully
validated or discard these failures. One accidentally broadened Catch negated-OR
selection exited139 in an unrelated serial geometry case; invalid selection and
raw receipt are preserved explicitly. Further frozen/unsteady/AD checks pending.
Local work is correctness only, low priority, one owned compute job at a time.
Two-core affinity plus OpenMPI yield is used; no local timing conclusions.

Temporary passive-communication tracing has been removed. Original/debug versions
and the trace binary are preserved in native_post_rebase_metric_v1/debug_passive_trace.
An M=N private-worker communicator trial also timed out and has been reverted;
its failed receipt is in private_worker_trial/run. Both production sources are
restored exactly. Original production archive build_v2 remains unchanged.
The MPI4 test-order interaction is unresolved; no sensor/BL guard is relaxed.

User explicitly requests selectable PREDICT snapshot count and fixed cadence
backward from the last adaptation step, plus temporal noise filtering. User chose
feature velocity/acceleration tracking rather than tensor-entry extrapolation.
Draft changes are isolated in /tmp/native-predict-multisnapshot; NOT applied to
repository sources yet. Planned options ADAP_PREDICT_SNAPSHOTS (default2), existing
ADAP_PREDICT_SEPARATION cadence, and ADAP_PREDICT_TEMPORAL_FILTER (default1). Extra
history fits centered/ridge-damped feature acceleration with an original-history
mismatch fallback to constant velocity; time-dependent transport traces each
future endpoint independently, preserving convex SPD interpolation and exponential
congruence. Additional optical flows and trajectories cost more; no performance
claim. Config/cadence and manufactured motion/SPD tests drafted, not compiled.
ADAP_HESSIAN_NOISE stays0. The latest user declines exclusive cluster nodes; the
old/new comparison uses usual shared nodes and chained jobs, timings approximate.
Comparison helper/README/fake-qsub checks are drafted in /tmp, not yet finalized.
Goal tracker remains PAUSED; these tasks are separately authorized.

## MPI4 timeout diagnosis corrected (2026-10-08)

Per-rank LD_PRELOAD traces on the unchanged archived build show matching native
collective sequences and continued progress. The supposed four-CPU mask 6,7,8,9
was also effectively two CPUs because this host has only CPUs0-7. Moving owned
ranks to CPUs4-7 completes GoalMPI->SteadyMetricReuse successfully, without any
production fix. Earlier timeouts do NOT establish a protocol deadlock and remain
preserved as failures. mpi_timeout_resolution.json records the diagnosis and
trace hashes. Communicator hypothesis and CPP tracing both reverted exactly.
Remaining baseline-merge correctness campaign now runs sequentially at nice10
on four valid CPUs; still no local performance assessment. New PREDICT draft
has boundary input validation and a manufactured jitter filtering test added.

## Baseline metric integration validated before prediction extension

validation.json aggregates the successful checks: metric/native MPI1/2/4,
18 frozen GG/WLS/QR RAE fields (targets12000/60000), independent sensor residuals
and composed diagnostics, forward/reverse non-OpenMP syntax, steady RAE N4M3,
frozen Euler-to-BL and BL-to-Euler N4M3, native WINDOW_AVERAGE CGNS vortex and
straight-wall SA with both BDF histories, MMG PREDICT/FIXED_POINT compatibility,
immutable RAE reference and accepted fixed-point output checks. Native PREDICT
and FIXED_POINT were explicitly unsupported; prepared native PREDICT smoke
stopped at that guard. The authorized prediction extension will admit PREDICT
only, retain native FIXED_POINT rejection, and add support tests. Invalid setup
attempts (duplicate config options, contradictory AD flags, applying native
sidecar/event checks to MMG) remain preserved with their corrected controls.
No mesh/transfer/metric numerical gate relaxed. Native audit defaults are intact;
only an explicitly selected MMG Euler control can omit native-sidecar/event checks.
Prediction extension is still unapplied and uncompiled.

## Prediction extension applied; full rebuild pending

Validated metric merge checkpoint5cb87667c20f7c590fab00aa373c7c056a2daca1
is committed locally, not pushed. Eleven reviewed files now extend PREDICT with
count/cadence, temporally filtered feature acceleration and time-dependent
transport. Native CheckSupport admits PREDICT; native FIXED_POINT still rejected
and its expected-failure test retained. Manufactured acceleration/SPD/jitter,
legacy two-frame equality, config schedules and MPI gather/scatter tests added.
Full j1 ABI rebuild (CConfig layout changed) is running in build_predict_v1.
Correctness cases prepared in predict_history_v1, including legacy MMG control,
native default-two, four-frame N1/M1 N2/M1 N4/M3 (CGNS) and a no-remesh donor
fixture for partial-window restarts. No new tests executed against the old ABI.
Cluster comparison drafts still pending final-source pins and final validation.

Prediction correctness preparation: `predict_history_v1/four_fixture_n1m1` now has ten steps (one actual native replacement at step 5), providing its real immutable reference sidecar and matching BDF restart states. Partial-window restarts will start at steps 7, 8, and 9 to exercise three-/two-/one-snapshot fallbacks without fabricating reference bindings. The sequential controller is saved as `integration_evidence/native_post_rebase_metric_v1/complete_predict_history_validation.py`. Invalid count/cadence/filter configurations and the retained native fixed-point guard are negative checks. Full ABI build remains in progress; no prediction smoke has run yet.
