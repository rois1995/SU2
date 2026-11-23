# Order RCFD1: real CFD between the remesh events (test code only; no production change; `SU2_CFD` of build647)

Author: reviewer, 2026-11-23 (checked against the source and your runners). For the main agent. The user sends it;
it is a user order. Copy this file with its SHA256 into `R/night_20261115/RCFD1/inputs/`.

## 1. Why

In each full run until now, the CFD stage after a remesh makes ONE iteration. So event 2 adapts to a solution that
was only interpolated. The user wants a real test: the CFD stage after each remesh runs 500 iterations, and event 2
adapts to a solution that was computed on the mesh of event 1. Two runs, one after the other: R-S (the fast path:
both options YES) first, then R-OFF (both options NO). This order accepts no mesh and changes no reference of
`AGENTS.md` section 2a. Where `AGENTS.md` section 2 asks for identical files in G5, read "write the hashes" for these
two runs.

## 2. What to do

1. Preparation (the same step as item 1 of `FP2_C1_CONTINUE_1.md`; if it is done already, verify the three hashes
   only). Verify that `git diff --stat 4730bd59ec -- Common SU2_CFD` is empty and that `SU2_CFD` has the SHA256
   `binary_sha256` of `R/night_20261115/FP2_C1/build647/build_status.json` (`b824f37e...6b4c`). Verify that no owned
   job runs. Put the test code back to the state of build647 (the test code of FP2-C part 2 step 0 stays in commit
   `ebf5bfff8f`): `git restore --source=4730bd59ec --staged --worktree -- UnitTests`; commit only these paths
   (`git commit -m "FP2-C1 continuation 1: test code = 4730bd59ec (test only)" -- UnitTests`). Build only
   `UnitTests/test_driver` with
   `build_fp1.build(work, 649, solver=False)` as `FP2_C2_0/run_c2.py` did for build648, in
   `R/night_20261115/FP2_C1/build649/`. Its `test_binary_sha256` must equal that of build647 (`689cfdac...cfd3`),
   its `source_pins_sha256` that of build647 (`0a0a2062...`), and `SU2_CFD` stays `b824f37e...6b4c`. Then the runners
   take `R/night_20261115/FP2_C1/build647` as the build directory. If the test-driver hash differs: stop this order,
   write it in the progress file, and wait.
2. Configs. Make `RCFD1/run_s.cfg` from `R/night_20261115/FP2_C1/run.cfg` (SHA256 `07b91a94...5382`) and
   `RCFD1/run_off.cfg` from `R/night_20261115/FX2/G5/build645/OFF/full645_run1/run.cfg` (SHA256 `5c0fe041...2d56`).
   Apply these edits and no other edit. Each old line must occur exactly one time as a whole line (compare complete
   lines, not parts of lines):
   a. `CFL_ADAPT= NO` -> `CFL_ADAPT= YES`;
   b. `ADAP_FLOW_ITER= ( 1 )` -> `ADAP_FLOW_ITER= ( 500 )`;
   c. `OUTPUT_WRT_FREQ= 1` -> `OUTPUT_WRT_FREQ= 100000`;
   d. `OUTPUT_FILES= ( RESTART )` -> `OUTPUT_FILES= ( RESTART, PARAVIEW )`;
   e. append the line `HISTORY_OUTPUT= ( ITER, RMS_RES, AERO_COEFF, CFL_NUMBER, LINSOL, ITER_TIME )`.
   Do not change `ITER= 1` (the stage before event 1 reads the converged seed; the isolated restart needs this
   line), `CFL_NUMBER`, `CFL_ADAPT_PARAM` (`( 0.5, 1.25, 0.5, 20.0 )`: the settings that made the seed), the
   `LINEAR_SOLVER_*` lines, `SCREEN_OUTPUT`, the file-name lines, `ADAP_SUBITER`, `RESTART_SOL`,
   `READ_BINARY_RESTART`, `ADAP_SENSOR`, `ADAP_LOOP`. Write the SHA256 of each new config in a `.sha256` file.
3. Runner (test code, R3). Copy `FP2_C1/run_fp2_b_g5.py` to `RCFD1/run_rcfd.py`. Change only: the modes (S and OFF);
   `config_for` (the new configs, with an assert that each equals its base plus the edits of item 2); the seal of the
   run: `workflow.seal_run(case, build['binary_sha256'], config, payloads, mode != 'OFF')` (as
   `FX2/run_fp2_b_g5.py:57`; in R-OFF the wall-law rows of all six ranks stay required); no six-file comparison and
   no whole-event comparison; the name and SHA256 of this order. The inputs of FP2-C1 stay where they are: in the
   copy, set `C1 = HERE.parent / 'FP2_C1'` and use `C1` (not `HERE`) for `scoped/`, `fixtures/`, `cases/`, `run.cfg`,
   `inputs/FAST_PATH_STAGE2_C1.md` with its `.sha256`, and `source_check.md`; only the work folders, the two new
   configs, this order and its own `source_check.md` are under `RCFD1/`. Keep the controller, the check helpers, the
   isolated restart and all their gates, with one exception: the stop on the line `NATIVE_SURFACE_REFINE` in mode OFF
   (a prediction from the one-iteration runs) becomes a record only. Make it so: in `run_rcfd.py`, wrap `core_source`
   of the campaign namespace before `campaign['prepare']` is called, so that the bound and pinned
   `full_controller.py` already contains the change; do not edit `full_controller.py` after `prepare`. The two
   replacements, each of which must occur exactly one time: `"report_only": FP1_ON, "records": guard_records` ->
   `"report_only": True, "records": guard_records`; and delete the three lines `if not FP1_ON:` /
   `stop_reason="p6c_one_sided_refinement_guard_failure"` / `break` that follow it. Write both in the report. Work
   folders: `RCFD1/G5/build647/S/` and `RCFD1/G5/build647/OFF/`.
4. Gates of both runs (the standing gates of G5, not changed): 5400 s for each remesh event; job peak not above
   19073 MiB and equal to the sum of the process peaks; the physical validator (q / J / size / geometry); mesh,
   restart coordinates and positive states; the transfer; continued CFD after each event; the isolated restart; the
   timer coverage of 95 % on each rank and the join agreement; owned proof faces SUM equal to the surface size;
   `fixedstorage=4120`. The six-file identity is NOT a gate in this order: write the SHA256 of the six files of each
   run. The controller also keeps its own stops: in R-OFF the gates of event 1 of the OFF mode (the first collapse
   of P6c, work 90,809,869 and `upper_work_pass`; event 1 does not change, so they are expected to pass); the job
   memory stop at 20,000,000,000 bytes; a rejected event; and a failed F3b or P7/P8 event gate stops the job at once
   (then the CFD of cycle 2 is lost; the CFD of cycle 1 is complete).
5. A failed gate, or a flow solution that diverges (NaN ends the job): one diagnosis pass (R5, read-only); keep the
   evidence; no own change of the config (it is ordered); do not repeat a failed run; label the run
   FAILED in the report. Then go to the next run: R-OFF does not depend on R-S. No revert (there is no code change).
6. Reports (not gates), `R/night_20261115/RCFD1/reports/RCFD1.md`:
   a. for each run and each cycle (0, 1, 2): points, cells, iterations, the last history row (the 5 rms values, CL,
      CD, the average CFL), the largest rms[Rho] of the stage and the first iteration with rms[Rho] 3 orders below
      it (or "not reached" with the drop that was reached), the range of CL and of CD over the last 100 iterations,
      the mean of the history column `Time(sec)` without iteration 0 of the stage (iteration 0 includes the
      preprocessing), and the `Solve [s]` and `Adapt [s]` of the Mesh Adaptation Summary (`Solve` includes the
      metric);
   b. for each event: the native maximum, the job peak, the smallest timer coverage, the proof faces; in R-S only,
      the rows `NATIVE3D_LOCAL_CORE` and `NATIVE3D_LOCAL_SURFACE` (R-OFF prints none); in both runs, the field
      `collapse_accepted` of the last row `NATIVE3D_LENGTH pass=collapse` of each rank;
   c. the mesh of event 1: in R-S, `adapted_mesh_adap_00001.su2` and its `.native_ref` beside those of
      `FP2_C1/G5/build647/S/full647_run1` (`fbe2c11411d2...`, `b13ca887e6c0...`); in R-OFF beside the N21 W
      reference (`104591aecdbc...`, `4fb162414013...`). They are expected to be equal (the
      stage before event 1 did not change). If one differs: write it as the first finding and as a question (R13);
      do not stop;
   d. the isolated restart: its rms[Rho], CL and CD at iteration 0 beside the last row of cycle 2 of the full run;
   e. one table: R-OFF, R-S and the seed on the original mesh (CL 0.269535, CD 0.0135359), with the points and
      cells beside the coefficients (the two paths make meshes of different size);
   f. the ParaView files of each cycle (expected: `flow_adap_00000.vtu`, `flow_adap_00001.vtu`,
      `flow_adap_00002.vtu` in the case folder; `flow.vtu` in the restart folder).
7. After the two runs: commit and push the evidence ("RCFD1: real CFD runs; no mesh accepted"), update the progress
   file, `HANDOFF_Codex.md` and `HowToProceed.md`, verify that no owned job runs. Then go on with the next order in
   the queue; if there is none, stop and wait for the user.
Each run is the only heavy work on the machine (R1). Expected: about 40 minutes for R-S and about 80 minutes for
R-OFF.

## 3. Not in this order

A production code change. A new build of `SU2_CFD`. A change of a gate, of the complexity target or of the solver
settings other than item 2. A third run.
