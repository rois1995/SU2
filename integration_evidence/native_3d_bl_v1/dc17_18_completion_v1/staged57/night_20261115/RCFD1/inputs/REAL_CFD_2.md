# Order RCFD2: two real-CFD runs with changed metric settings (config only; no production change; build647)

Author: reviewer, 2026-11-23 (checked against the source and your runner). For the main agent. The user sends it; it
is a user order. Copy this file as `R/night_20261115/RCFD1/inputs/REAL_CFD_2.md` and write
`R/night_20261115/RCFD1/inputs/REAL_CFD_2.sha256` with the single line `<sha256>  REAL_CFD_2.md`.
Do not stop a job that runs (R12). Start this order when the run or gate that executes now has ended and its
evidence is committed. After this order, return to the FP2-C1 continuation at the step where you stopped (item
1.2(a) or later, as your progress file says).

## 1. Why

The user looked at run R-S of RCFD1: the shock is not refined. The reviewer measured the cause: the metric does not
ask for it. With the norm p = 1 (`ADAP_NORM` is not set) the far field gets about half of the complexity and the
shock band 0.7 %; the requested size across the shock is 24 mm, the same as in the original mesh. And near the wall
the gradation (1.3) turns the fine direction of the metric away from the shock normal. This order makes two runs as
a test of the target settings: R-SM with p = 2 and a smallest size of 3 mm; R-SG with the same and a gradation of
1.6. It accepts no mesh and changes no reference of `AGENTS.md` section 2a.

## 2. What to do

1. Preconditions and configs. Before each run verify: `git diff --stat 4730bd59ec -- Common SU2_CFD` is empty,
   `SU2_CFD` has the SHA256 `b824f37e...6b4c` (`FP2_C1/build647/build_status.json`), and
   `FP2_C1/scoped/build647/fixtures.json` and `FP2_C1/cases/build647/lab_acceptance.json` still verify
   (`prerequisite_sources`). Make the two runs before any production edit of the queue; if one of these checks
   fails, do not run, write it in the progress file and wait.
   Make `RCFD1/run_sm.cfg` from `RCFD1/run_s.cfg` (the config of your run R-S; verify its `.sha256`) with these
   edits and no other edit:
   a. `ADAP_HMIN= 1E-6` -> `ADAP_HMIN= 3E-3` (the old line occurs exactly one time as a whole line);
   b. append the line `ADAP_NORM= 2`.
   Make `RCFD1/run_sg.cfg` from `RCFD1/run_sm.cfg` with one more edit:
   c. `ADAP_HGRAD= 1.3` -> `ADAP_HGRAD= 1.6` (the old line occurs exactly one time as a whole line).
   Do not change `ADAP_HESSIAN_NOISE= 0`, `ADAP_COMPLEXITY`, `ADAP_SIZES` or another line. Write
   `RCFD1/run_sm.sha256` and `RCFD1/run_sg.sha256` (one line `<sha256>  <file name>`).
2. Runner (test code, R3). Copy `RCFD1/run_rcfd.py` to `RCFD1/run_rcfd2.py` and make the changes only in the copy.
   Do not edit `run_rcfd.py`, `source_check.md`, `run_s.cfg`, `run_s.sha256` or `inputs/REAL_CFD_1.*`: their SHA256
   are pinned in `G5/build647/S/helper_pins.json` and `G5/build647/OFF/helper_pins.json` and `verify_run` checks
   them. Write your source check of this order in `RCFD1/source_check_rcfd2.md`. In the copy:
   `MODES = ('S', 'OFF', 'SM', 'SG')`; in `config_for`, a branch for `SM` and one for `SG`, placed before the
   existing `else` (which is the OFF branch): `SM` builds the bytes of `config_for('S')` with the edits a and b and
   asserts that they equal `run_sm.cfg` and that `run_sm.sha256` reads `<sha256>  run_sm.cfg`; `SG` builds the
   bytes of `config_for('SM')` with the edit c and asserts the same for `run_sg.cfg` / `run_sg.sha256`; in
   `--self-check` replace `== (mode == 'S')` by `== (mode != 'OFF')`; constants
   `ORDER2 = HERE / 'inputs/REAL_CFD_2.md'` and `ORDER2_SHA`, checked in `verify_order` like `ORDER`; in `run()` add
   `ORDER2`, `ORDER2.with_suffix('.sha256')` and `HERE / 'source_check_rcfd2.md'` to `source_paths`. The modes `SM`
   and `SG` are runs with both options YES: the binding of the campaign and `seal_run(..., mode != 'OFF')` treat
   them as mode S. Work folders `RCFD1/G5/build647/SM/` and `RCFD1/G5/build647/SG/`. Run
   `run_rcfd2.py <FP2_C1/build647> --only SM`, then `--only SG`.
   In `report_events.py`: add `'SM': ('fbe2c11411d2', 'b13ca887e6c0')` and the same pair for `'SG'` to
   `EXPECTED_PREFIXES` (the event-1 files of R-S); in `reference()` take the S branch for
   `mode in ('S', 'SM', 'SG')`; `collect()` iterates over `('S', 'OFF', 'SM', 'SG')`; in `identity_findings` a row
   of mode `SM` or `SG` is a finding when `row['equal'] is True` (the settings did not act), the other modes
   unchanged; in `markdown()` replace `if mode == 'S':` by `if mode != 'OFF':`, set
   `ref_label = 'C1 S (= R-S event 1)' if mode != 'OFF' else 'N21 W'`, and write the result of an `SM` or `SG`
   identity row as `different (by design)` or `equal (finding)`. In `report_flow.py` (line 128) iterate over
   `("S", "OFF", "SM", "SG")`. Run `report_events.py --self-check` unchanged.
3. Run R-SM with its isolated restart, then run R-SG with its isolated restart. The gates of each run are those of
   `REAL_CFD_1.md` section 2 item 4 (the standing gates; the six-file identity is not a gate: write the SHA256 of
   the six files). The mesh of event 1 differs from that of R-S by design (the metric changed): it is not a
   question. If the two event-1 files of R-SM or R-SG are EQUAL to those of R-S, or those of R-SG are EQUAL to those
   of R-SM, write it as the first finding (the setting did not act).
4. A failed gate, or a flow solution that diverges: one diagnosis pass (R5, read-only); keep the evidence; no own
   change of the config; do not repeat a failed run; label the run FAILED in the report. Then go to the next run:
   R-SG does not depend on R-SM. No revert (no code change). (`ADAP_HGRAD` other than 1.3 was never run in the
   native 3D path: a failure of R-SG is a result of this test.)
5. Report (not gates), `R/night_20261115/RCFD1/reports/RCFD2.md`, R-SM and R-SG beside R-S:
   a. from the log of rank 0: the row `Its metric makes the next mesh: ...` before event 1 and before event 2 (it is
      not printed after cycle 2; expected `complexity 50000, sizes 0.003 to 100, norm 2, aspect ratio up to
      1e+06`); and for each of the three metric computations the rows `Maximum density: ...` (expected not above
      3.7037e+07), `Maximum cell AR: ...`, `Sensor metric complexity before gradation: ...`, `Sensor-only donor
      complexity after gradation: ...`, `Native metric work: ...` and `Native sensor gradation: ...`;
   b. the items a, b, d, e, f of `REAL_CFD_1.md` section 2 item 6 for each run (flow history of each cycle; the
      events; the isolated restart; the table with R-S and the seed; the ParaView files);
   c. for each event: the native maximum beside R-S, the points and cells, the surface faces of the wing marker.
6. Commit and push the evidence ("RCFD2: real CFD runs with p = 2; no mesh accepted"), update the progress file,
   verify that no owned job runs. Then return to the FP2-C1 continuation at the step where you stopped.
Each run is the only heavy work on the machine (R1). Expected: about 35 to 45 minutes for each run.

## 3. Not in this order

A production code change. A new build. A change of a gate. A third run. `ADAP_HESSIAN_NOISE` (the native path
permits only 0).
