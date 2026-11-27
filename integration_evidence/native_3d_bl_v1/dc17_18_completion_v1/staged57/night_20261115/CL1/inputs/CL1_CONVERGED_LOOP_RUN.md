# CL1: one capability run of mode S with the flow solved on each adapted mesh, with ParaView output

Author: reviewer, 2026-11-27 (reviewed by Codex). For the main agent. The user sends it; it is a user order.
Copy this file with its SHA256 into `R/night_20261115/CL1/inputs/`, together with the measure script
`reviewer_handoff/scripts/s6/shock_refinement_measure.py` and its SHA256 (the SHA256 is in section 5).
Start only after the item FP3-E has its final label ("accepted", "FAILED" or "OPEN") and its owned handles are
terminal (R12). Then resolve the S reference in force from `AGENTS.md` section 2a. Use the solver path and SHA256
from that reference's sealed build and run records. Record the checkout commit and the binary's source commit
separately. Authenticate the template configuration and its three input files (`m6.su2`, `m6.su2.native_ref`,
`baseline_euler_seed.dat`) against the provenance of D S1 (`provenance.json`). Start no build.
Short names: R, N, "mode S", "standing gates" as in the order FP3-A (your copy in `N/FP3_A/inputs/`), called
"A". "The template" = `N/FP3_D/G5/build678/S1/trial1/full678_run1/run.cfg`. "Event k" = the adaptation that
makes mesh k (k = 1..4); "cycle k" = the flow solve on mesh k (cycle 0 = the input mesh).

Scope. NO production change. A new run configuration and a new controller (test code, R3) for ONE run. The run
is a capability observation: it gives no reference and changes no gate, budget or reference in force.

## 1. Why

The user asked why the shock of the M6 wing is not resolved in the adapted meshes. Reviewer studies (read-only):
`reviewer_handoff/scripts/s6/study_converged_loop.md` (with `out/`).
- The template solves the flow for ONE iteration on each adapted mesh (`ITER= 1`). Adaptation 2 therefore uses
  the solution of the input mesh, transferred: the shock never gets sharper.
- On the FP3-D meshes the shock stays at the input resolution: the extent of the sonic-crossing edges along the
  in-surface shock normal is 0.0172 / 0.0170 / 0.0166 (input / mesh 1 / mesh 2). The metric asks for 0.032 to
  0.042 along the shock normal (coarser than the mesh has); the resolution goes to the leading edge.
- The question of this run: does a converged flow on each adapted mesh make the metric (Mach sensor, 50k) ask
  for, and the mesher give, a finer shock?

| Assumption | State |
|---|---|
| The loop runs `ADAP_FLOW_ITER` iterations per adapted cycle and stops a cycle early when all `CONV_FIELD` criteria hold | read in the source (study s6 Q1, `CSinglezoneDriver.cpp:571-640, 2205-2218`, `COutput.cpp:1100-1204`) |
| Each cycle writes `restart_flow_adap_0000N.dat`, `flow_adap_0000N.vtu`, `surface_adap_0000N.vtu` | read in the source; `.vtu` after an ACCEPTED native mesh replacement is NOT KNOWN (no run evidence) |
| Fixed CFL 5 with the linear solver 1e-6 / 20 gives a quiet Euler state; the scheme does not reach rms -10 | MEASURED on the input mesh (seed runs v2 / v3, study s6 Q3) |
| Fixed CFL 5 is stable from a transferred state (rms about -2) | NOT KNOWN |
| Iterations to the Cauchy criterion on the adapted meshes | NOT KNOWN (cap 2000) |
| Native time and memory of events 3 and 4 | NOT KNOWN (events 1 / 2 of FP3-D: 477 / 157 s; job peak 9.3 GB) |
| Whole run about 1-2.6 h | an estimate (study s6 Q4) |

## 2. The configuration

Copy the template to `N/CL1/run/run.cfg` and change ONLY these keys (add a key that is absent):
```
ADAP_SIZES= (50000)
ADAP_SUBITER= (4)
ITER= 1
ADAP_FLOW_ITER= (2000)
ADAP_FLOW_CFL= (5.0)
CFL_ADAPT= NO
LINEAR_SOLVER_ERROR= 1E-6
LINEAR_SOLVER_ITER= 20
CONV_FIELD= (LIFT, DRAG)
CONV_CAUCHY_ELEMS= 100
CONV_CAUCHY_EPS= 1E-7
CONV_STARTITER= 300
HISTORY_OUTPUT= (ITER, RMS_RES, AERO_COEFF, CAUCHY, LINSOL, CFL_NUMBER)
OUTPUT_FILES= (RESTART, PARAVIEW, SURFACE_PARAVIEW)
OUTPUT_WRT_FREQ= (100000)
```
Keep every other key of the template (mesh, seed, `ADAP_*` native keys, `VOLUME_OUTPUT`, `MARKER_PLOTTING=
(WING3D)`, `VOLUME_FILENAME= flow`, `SURFACE_FILENAME= surface`, `SCREEN_OUTPUT`, 6 ranks, mode S keys). Write the
diff of the two files in the report. Launch environment: start with the launch environment of D S1
(`provenance.json`); remove every inherited `SU2_NATIVE_*` variable; leave `SU2_NATIVE_CONFORMING_WITNESS` unset;
export `SU2_NATIVE_GEOMETRY_WORK_TRACE=0` and `SU2_NATIVE_FP3_PROBE=0` to all six ranks; keep the single-thread
settings; update the case-local paths; receipt the effective settings of every rank (as in A). In the R4 check confirm each key name in `Common/src/CConfig.cpp` (an unknown key is a contradiction under R4)
and that `CONV_FIELD` names LIFT and DRAG of this solver (an unknown field is skipped silently,
`COutput.cpp:1107`); the first cycle's convergence summary in the log must list both.

## 3. The controller and the gates

A new controller under `N/CL1/` (test code, R3). Do not reuse the injected identity chain of FP1 / P3 / FP3; reuse
its per-event gate functions in a loop over the events 1 to 4 (the event-2 assumptions listed in study s6 Q5
items 1-5 and 8-10 must not remain). One owned heavy job, nice 19, the host preflight of R1 (the 6 ranks are 6
heavy processes; wait until no other heavy process of yours runs). Watchdog at 21600 s: terminate and reap the
verified owned process group with bounded escalation (as the template controller does) and record all terminal
handles; a `timeout` alone is not evidence of terminal handles.

Event boundaries. For each rank, partition the native evidence by the terminal `root=remesh` timer of each
event. Use the LAST worker `stage=exit` within each event (one event can run two worker stages after a
normalization correction, `NativeRemeshRun.inc:567-597`) and pair it with that event's refine detail and native
summary. A completed run has four completed event packets on all six ranks. Bound the transfer text of event k by
the log lines `Cycle {k-1} of 4` and `Cycle {k} of 4`. Parse the seven numeric transfer columns before any optional
`bounds relaxed` suffix and report that suffix separately.

Gates (each per event k = 1..4, with the functions of the standing G5 gates):
1. 5400 s for each event (native rank total); the exit certification sums (q / J / size / geometry bad = 0,
   counts consistent).
2. The physical validator on mesh k and restart k (positive volumes, restart coordinates = mesh to 1e-12, finite
   positive density and internal energy).
3. Transfer of event k: conservative residuals <= 1e-10, zero inadmissible states, ordered transfer /
   replacement / publication markers.
4. Continued CFD on mesh k: in `history.csv` (named columns), cycle k has finite rows for `Inner_Iter` = 0 up to
   the driver summary's `Iterations - 1`, once each, with `Adap_Cycle == k` and contiguous `Adap_Iter`. Parse the
   solver exit message within the log section of cycle k. Retire the old unbounded screen regex.
5. 95 % timer coverage on each rank and the join agreement of event k; owned proof faces SUM equal to the
   surface size.
6. Memory: for each event the exact check `job_peak_rss == SUM(process_peak_rss)` and the limit 19073 MiB, as
   today. In addition, record one terminal high-water mark for each of the six solver ranks through the final CFD
   and output (a per-rank wait / rusage receipt) and gate their sum against the same limit. Missing terminal peak
   evidence leaves the whole-job gate unevaluated. Polling samples are diagnostics.
Not evaluated (write it): the six-file identity, the first-collapse identity (report only, if computed), the
two-run rule, the isolated restart.

Failures. This item has no production change: a failed gate is a capability RESULT. Correct controller setup
errors under R3 before the one solver launch. After the launch, correct postprocessing only from the saved files.
Record the termination cause separately from the wait status. A proven result failure (a failed gate, a
recorded native rejection, solver divergence, a spontaneous nonzero solver exit) is FAILED even if later evidence
is cut. A watchdog-triggered exit is OPEN unless independent evidence establishes a result failure; missing
evidence that prevents classification is OPEN. An interruption message establishes neither convergence nor a cap
exit. Mark unvisited events "not evaluated". Preserve all evidence,
publish no reference, make no repeat solver run and no revert, and do not change the configuration.

## 4. The report (no gate)

`N/CL1/reports/CL1_<build>.md` and `.json`:
1. For each cycle 0..4: iterations used; "converged" or "cap reached", decided by the solver exit message of the
   cycle (quote it; the driver gives convergence priority when both hold at the last iteration); final Cauchy values of CL and CD; rms[Rho] at the first and last row and the drop; CL, CD;
   points and cells of the mesh; solve seconds.
2. For each event 1..4: native time, driver adapt time, job peak memory, host load at start and end.
3. The shock measure: run the copied script unchanged:
   `python3 shock_refinement_measure.py m6.su2 restart_flow_adap_00000.dat adapted_mesh_adap_00001.su2
   restart_flow_adap_00001.dat ... adapted_mesh_adap_00004.su2 restart_flow_adap_00004.dat` (mesh N with restart
   N). Put its JSON lines in the report and show for each cycle: B (count, realized shock-normal size, Mach jump
   per edge, metric size along the shock normal and along the wall normal; the leading-edge point count and the chordwise,
   wall-normal and minimum metric sizes there) and C
   (for each station: shock x/c, Cp jump, 10-90 % width over chord, points inside the width, lambda flag).
   Pair "requested" (metric of restart k) with "realized" (mesh k+1).
   On a partial run pass only the available mesh / restart pairs and mark the missing cycles "not evaluated".
4. The output files for the user: list with path and size, for each cycle, `flow_adap_0000N.vtu`,
   `surface_adap_0000N.vtu`, `restart_flow_adap_0000N.dat` and, for N >= 1, `adapted_mesh_adap_0000N.su2` with
   its `.native_ref` (hash each). Do not
   delete or move them after the run. If a `.vtu` is missing or empty after an accepted mesh replacement, write it
   as a finding (not a gate) with the log lines of the output step.
5. The same numbers beside FP3-D S1 where it has data (cycles 0-2, no CL / CD Cauchy history;
   `N/FP3_D/G5/build678/S1/trial1/full678_run1`; its measure is in
   `reviewer_handoff/scripts/s6/out/measure_fp3d.jsonl`).
6. For each event, whether the local surface core actually ran (the worker checks eligibility).

Labels: "CL1: converged loop run DONE" (all gates of all four events passed and all four adapted cycles report
"All convergence criteria satisfied"), "CL1: loop run DONE; convergence cap reached on cycles <list>" (all gates
passed, one or more adapted cycles reached the 2000 cap; cycle 0 reaches its cap of 1 by design and is reported
apart), "CL1: converged loop run FAILED at
event <k>, gate <n>" (when both are known; otherwise "CL1: converged loop run FAILED at <stage>: <reason>", e.g. at
startup or in cycle 0), or "CL1: converged loop run OPEN" (watchdog or cut evidence). Commit the controller, the
configuration, the receipts and the report (not the large output files, which stay in the run folder; give
their hashes in the receipt). Update the progress file. No change to `AGENTS.md` section 2a. Then STOP.
The rules R1, R3, R6, R7, R8, R11, R12, R13 hold. Expected machine time: 1 to 3 hours.

## 5. Inputs

- Measure script `reviewer_handoff/scripts/s6/shock_refinement_measure.py`, SHA256
  `b74990859163f7936d897f886928eb79200d63ce27322fa59864bf96b80dc1ee`.
- Study `reviewer_handoff/scripts/s6/study_converged_loop.md` (context, not an order).

## 6. Not in this order

A production change. A change of the metric, the sensor, the norm, the complexity, the gradation or the limiter.
A run at 100k. A second run. A new reference.
