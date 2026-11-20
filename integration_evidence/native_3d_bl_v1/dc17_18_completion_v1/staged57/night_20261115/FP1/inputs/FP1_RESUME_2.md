# FP1 resume 2: the move of a point must not fail on a flat old cell; time fields; a replay of event 1 with the core ON

Author: reviewer, 2026-11-20, 10:30. For the main agent. The user sends it; it is a user order.
Base: `931b306263` ("Close FP1 with OFF passed and ON failed"). All rules of section 1 of `AGENTS.md` apply.
R = `integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57`. "The text" = `FAST_PATH_STAGE1.md`; "resume 1" =
`FP1_RESUME.md` (your copies in `R/night_20261115/FP1/inputs/`). Copy this file with its SHA256 into the same folder.
Short names as in resume 1 (NRR, Pass, Changes, Mesh, Shift, Hdr). Evidence: `R/night_20261115/FP1/resume2/`.

Why. The stop of the ON run was correct, and your diagnosis is right: the cause is in the text (section 6.6), an error
of the reviewer. `MovePoint` gives all old cells around the point to `EvaluateCavitySample`. That function throws
"Degenerate metric donor" when the long-double determinant of ONE old cell is zero, also when that cell is valid by the
exact `Orientation` (a very flat cell). After the collapses such cells exist, and the quality work moves exactly their
points. Second fact of the failed run: on rank 0 the first pass was open for 413 s, 377 s of it waiting for another
rank. The core is far too slow on one rank at least, and the run printed no pass row, so the place is not known.

This order: (1) corrects the rule, (2) adds time fields to the print, (3) runs the core on the frozen data of event 1
without the CFD. It does NOT run G5. After the report the reviewer designs the speed step with the numbers.

## 1. Part G1: the tensor of a moved point (correction of section 6.6 of the text)

New rule: the tensor at the new place is the P1 value in the old cell around the point with the largest minimum weight,
AMONG THE OLD CELLS WHOSE WEIGHTS CAN BE COMPUTED. A cell whose weights can not be computed is not used. If no cell is
left, this step of the move is refused. A refused step is not a failure.

Edit in `Mesh::MovePoint` (Changes:357), inside the loop over the steps, in the place of the one line
`candidate.tensor = SU2Native3D::EvaluateCavitySample(donors, candidate.node.p).sensor;`:
1. Make the list `usable`: each donor of `donors`, in the same order, for which
   `SU2Native3D::MetricAffineWeights(donor.cell, candidate.node.p)` (`CNativeMetricTransport3D.hpp:16`) returns four
   finite values. A donor for which this call throws `std::invalid_argument` is not usable (catch exactly this type,
   around exactly this call).
2. If `usable` is empty: `refusal = Refusal::Numeric` (a new value of the enum `Refusal`, Hdr:63), count it
   (`stats.Count`, a new print counter `refused_numeric`), and go to the next step. No point is changed.
3. Else `candidate.tensor = SU2Native3D::EvaluateCavitySample(usable, candidate.node.p).sensor;` and the rest as today.
- Put steps 1 to 3 in one small function of the core (proposal: `bool SampleMovedTensor(const std::vector<DonorCell>&,
  Point, Tensor&)`), public in Hdr, so that a fixture calls it.
- The memory of refusals (F2) treats `Numeric` as each other refusal of a move (it depends only on the cells around the
  point). `Refusal::Memory` stays the only reason that is not remembered.
- Do not change `CNativeMetricTransport3D.hpp` and do not add another catch. If a pass still ends with an exception of a
  production function: report the function and the message (10.5 of the text holds), do not add a rule of your own.

## 2. Part G2: time and count fields (print-only, R11: the clock is read only when `report` is true)

Add to `PassStats` and to the row `NATIVE3D_LOCAL_CORE` (in `PrintLocalPass`, NRR:727), after `skipped_memory=`:
- `refused_numeric=`;
- `seconds_build=` (arrays from `Owned`), `seconds_collapse=`, `seconds_split=`, `seconds_quality=`, `seconds_check=`
  (the self-check and `ReportResult`), `seconds_write=` (writing back);
- `evaluated_collapse=`, `evaluated_split=`, `evaluated_quality=`: the count of entries that were really evaluated
  (not deferred, not skipped);
- `round_changes=`: one text field, for each round of the length work `c<accepted collapses>s<accepted splits>` with
  the sweeps in brackets, joined by commas (example `c5120(14)s9020(9),c310(6)s122(4)`);
- `quality_changes=`: the accepted quality changes of each quality sweep, joined by commas.
No decision reads these values.

## 3. Fixtures (new test cases, new tags; R3)

1. `SampleMovedTensor` with a flat cell. The cell with the points (0,0,0), (1,0,0), (0,1,1+e), (0,1-e,1), e = 2^-52 (if
   its `Orientation` is negative: exchange the last two points) is valid by the exact `Orientation`, and
   `MetricAffineWeights` throws for it (its long-double determinant is zero). Assert both facts. Then: (a) donors = this
   cell and one normal cell that holds the sample point: the function returns true and the tensor of the normal cell's
   P1 value; (b) donors = this cell alone: it returns false; (c) donors without a flat cell: the same tensor bits as
   `EvaluateCavitySample`.
2. All fixtures of the text and of resume 1 pass again (their counts can change only by the new refusal kind: report).

## 4. Replay of event 1 with the core ON (one heavy run, R1; no CFD)

The frozen-volume test `[.][NativeFrozenVolume3D]` (`CNativeRemesher3D_tests.cpp:343`) at 6 ranks, as step 1 of part B of
FP0: mesh `G5/m6.su2`, target from `G5/restart_flow_adap_00000.dat`, with G5 =
`R/night_20261115/N4/section6_3_after_N11/round_8_5t_P/G5/full587`. The frozen config is the one of FP0 part B with the
one more line `ADAP_NATIVE_LOCAL_CORE= YES` and timings on (a copy in your evidence folder; record its SHA256). No
state capture is necessary. The event has its normal limits (5400 s, memory).
Report:
- the rows `NATIVE3D_LOCAL_CORE` and `NATIVE3D_LOCAL_SHIFT` of each rank and pass, complete;
- the seconds of the core for each rank and where they are (the fields of part G2);
- the event: accepted or the first message; the native wall seconds; cells and points at the entry of the engine and at
  the exit; the engine's accepted collapses and splits and its rounds; its seconds for each phase; the smallest timer
  coverage; the job peak. Beside them the same numbers of event 1 of the OFF run of this morning
  (`R/night_20261115/FP1/resume/G5/OFF`, native 1,818.319 s) and of the OFF replay of FP0 part B (1,991.772 s).
If the core fails again: R5 diagnosis as written, with the pass rows of the ranks that finished, and stop.
If the event is not accepted after the core finished: that is a result of this replay; report the first failing
number and the state; do not change code.
Optional second replay, only if the first is accepted and the frozen test takes these inputs by its environment
variables without a change of test code: event 2 (mesh `G5/adapted_mesh_adap_00001.su2`, target from
`G5/restart_flow_adap_00001.dat`). The same report.

## 5. Gates of this order

1. Build without a new warning. `git diff f36b66eadd` over the code tree: only the core files, `PrintLocalPass` in NRR
   (the new fields), test files, `UnitTests/meson.build`.
2. The fixtures of section 3 pass. G1 identical.
3. OFF check of the new binary: G2 and GQ by the rule of resume 1 section 1 (the OFF path has no changed statement; the
   G5 OFF run of this morning stays the proof for the six files).
4. No G5 run in this order. The item FP1 stays open: "OFF passed; ON: correction 2 in work".
Commit and push ("FP1 G"). Report `R/night_20261115/FP1/reports/FP1_<build>_resume2.md`, the progress file, the
handbooks as usual. Verify that no owned job runs, stop and wait for the user.

## 6. Not in this order

A speed change of the core (it comes with the numbers of section 4). A change of thresholds or limits. G5. FX1, FP2-0.
