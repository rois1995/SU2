# Native3D field accounting draft

Repository: /media/rausa/4TB/SU2_Versions/SU2_NativeIntegrated.
Branch: codex/native-3d-core; predecessor9fddd081a8435e83a7f069043608d2b1f68ac437.
Status: C++ BUILT and SERIAL_CHECKED; MPI2/4 and fixed-work envelope/restart qualification remain pending.

Implementation/evidence checkpoint 542ba6375219cb84ddc4c7b8ab1bc02e42fe4a3b pushed to origin/codex/native-3d-core; full remote SHA verified. Serial checks pass; MPI and broader cluster qualification remain pending.

The original FrozenField already counts true cache requests/hits/misses,
invalid queries, query failures and FIFO evictions. EngineStats/export omitted
misses and invalid/failure/eviction counts. Engine measurement and bridge sampling
aggregated only at the end of a successful try-block. A composition or subsequent
metric/topology exception therefore discarded queries performed before rejection.
The nonstreamed private engine path had the same omission; streamed measurement
already aggregated its surviving field after catch.

This draft adds scalar aggregate fields and exports them in existing detailed
engine/bridge records. Small local lifetime guards retain selection/bridge field
statistics when the surrounding block throws; private engine accounting moves
after catch using its existing pointer. No extra query, allocation or collective
is introduced by the guards. FrozenField interpolation/cache/composition,
sensor-only donors, geometric composition hooks, targets/operators/guards,
BL/fade/noise policy and2D/MMG implementation remain unchanged. Construction that
throws before a FrozenField exists still has no object statistics to aggregate;
outer exclusive phase elapsed time remains the cost envelope.

Tests extend existing selected cases, keeping69 test cases/rank expected:
- An injected engine composition failure at selection must preserve accepted
  input and returned private cells, publish zero edits and retain query/failure/
  composition counts with detailed timing OFF and ON.
- WorkingWeights and ReaderSlices injected composition failures must leave the
  output sentinel and accepted CFD mesh/solution/history unchanged, with actual
  attempted requests/failures retained under OFF and ON and M<N inputs.
- Successful automatic repair and weighting check closure and eviction bounds.
These C++ assertions PASS in the serial MPI1 selected regression; MPI2/4 remain pending.

Shared lifecycle/envelope/cluster auditors require integer nonnegative finite
counts, requests=hits+misses+invalid_queries, invalid_queries<=failures<=requests,
evictions<=misses and compositions<=requests-invalid_queries. This requirement
covers successful records and inactive ranks; C++ failure assertions cover
rejected calls. Five cache-corruption controls augment the lifecycle auditor.
Historical logs that do not contain the full schema should be checked with their
saved/bundled original auditor, not upgraded and called a new profiling certificate.

package_checks_v6 passes12 lightweight checks in0.303s, including synthetic valid
and corrupted counter arithmetic, missing counters, nonfinite/negative values
and shared nested-stage validation, plus existing packaging/submission/failure
retention/single-pilot controls. Python/shell syntax and diff checks pass; current
source/artifact hashes were reverified. This is not C++/MPI/numerical evidence.

REFERENCE_ADMISSION_MATRIX freezes15 exact expected source changes against both
retained reference-lookup baselines, explicitly including this accounting fix,
auditors and the already-published n8 fixture/runner. The original n4 fixed-work
and n2 persisted-restart cases/targets remain unchanged. Qualification still
requires full68-object configured regression,69 selected cases per MPI1/2/4 rank,
serialMMG48 assertions, identical26 raw fixed-work geometry/source/state/readback
files, unchanged accepted operation counts, independent geometry/metric/current+
BDF/SU2CGNS/restart checks, closed phases and actual production/member pins.
This combined pending validation is not an isolated timing attribution study.

reference_admission_build_v1 retains source snapshots and actual host preflight.
The wrapper ended FAIL because its resource gate refused before creating any
compiler PID/stage: sampled idle27.33%, but one-minute load exceeded eight-core
capacity. No owned compiler, MPI, CFD or scheduler job was launched. All source
and retained artifact pins were reverified. Compiler scratch was an empty owned
4TB folder; no new /tmp pressure or foreign process/data changes.

reference_admission_build_v2 subsequently passed the host resource gate
(idle27.33%,load1=6.76), completed all19 configured build steps with nice19/j1,
and pinned all68 objects/source/binary without drift. reference_admission_serial_v1
then passed serialMMG48 assertions and all69 selected cases/9996 assertions,
including the new constructor/import/reload and counter-failure controls. Only
one numerical rank/job ran at a time, with thread1 and4TB scratch. No MPI2/4 or
manufactured envelope/restart correctness/performance result is claimed here.

Next: complete the frozen MPI1/2/4 regression/matrix and qualifier before claiming
the full reference-admission checkpoint. The cluster package is now ready for
a current on-cluster build/freeze and user launch; no cluster job was submitted. Preserve failed attempts. FullGoal1 ACTIVE;
last validated production1fafe9e83c. Complete actual instrumentation overhead,
privateCPU/wait/traffic/allocator calibration and larger practical-limit/scaling
runs remain open. No new complete3DBL/curved/fullunsteady/PDE/affordability claim.

Publication checks: source/docs pass Git whitespace checks. Two retained raw stdout files have trailing blank lines; these hash-pinned logs are preserved unchanged.
