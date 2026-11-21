# FP2-A correction 1: step L of the surface without the progress test

Author: reviewer, 2026-11-21 (checked against the source). For the main agent. The user sends it; it is a user order.
It changes ONE rule of `FAST_PATH_STAGE2_A.md` ("the text"; your copy in `R/night_20261115/FP2_A/inputs/`). All other
parts of the text stay. Copy this file with its SHA256 into the same folder.
R12 holds: do not stop a job that runs. Apply this correction at your next sub-step boundary.

## Why

Your part A is correct and it passed. LAB-S showed a fault of the text, not of your code: step L left 1,163 long
surface edges (`left_long=1163`, `refused_size=1163`), all refused by the progress test of `Split` (text 4.3 step 5 and
section 6, "the change is `SplitSurface` with `progress_test`"). The cause: that test asks that EACH new edge is
shorter than the split edge, and the new edges of a surface split are the edges from `m` to each point of each ring
cell: the surface points and the points in the volume. Step L runs before the volume work, so the volume near a coarse
surface part still has longer edges; this is the probable reason of the refusals (your counters do not say which new
edge failed). The ends of these edges then stay `unresolved`, the volume around them stays `Blocked` for the core, and
the engine must do all of it. The text also put each refused edge in a set for the whole pass, so it was not tried
again. Your prototype of FP2-0b had no such test in step L and ended after 7 sweeps with 2,415 splits (2,380 of the walk
kind and 35 feature splits).
The progress test was made (FP1 resume 4, part H6) for volume splits near a surface that could not be split. A surface
edge can now be split, so the cause of that rule is not present in step L.

## 1. The change

1. Step L calls `SplitSurface` WITHOUT the progress test (as step D does). Remove the parameter `progress_test` and
   step 5 of text 4.3, or keep the parameter and give `false` in both steps; your choice (R9). If you remove the
   parameter: remove its argument in EACH call too (in your signature the next parameter is `bool report`; a `false`
   that is left in a call would then switch the report off).
   `options_.split_progress` and the test in `Split` (the volume) do not change.
2. Nothing more changes: the order of step L (L descending, smaller id, larger id), its stamp and deferral rule, its
   work list, its limit of 50 sweeps, its refused set (now only for the other reasons of 4.3 and 5.2) and its border
   set stay.
3. The field `refused_size=` of the row `NATIVE3D_LOCAL_SURFACE` stays in the row (expected 0 now).
Expected (a report, not a gate): `left_long` of LAB-S goes down to a small count (edges at refusals of the walk or of
another test); the surface splits of step L are near those of your prototype (2,415); the cells at the end of the pass
change.

## 2. What to repeat

1. Fixtures 2 and 3 of the text, fixture 4 (IN1) and LAB-S, with the pass conditions of the text. Report the printed
   counts beside those of build621 (no fixture pins a count of step L). In the report of LAB-S write the new row beside the row of
   build621, and the count of cells that are `Blocked` at the end of the surface steps before and after this change if
   you can count it without a production change (if not: leave it out).
2. If step L reaches its limit of 50 sweeps in LAB-S, or the count of cells at the end of the pass is more than 1.5
   times the 469,411 cells of build621 (this is an alarm for a runaway, not a quality rule): do not go on to part C;
   write the numbers in the report, commit the evidence with a label that says "stopped" (R8: not "passed"), stop and
   wait for the user.
3. Build without a new warning; all fixtures of FP1; G1 identical. Commit and push ("FP2-A part A correction 1").
4. Then continue the text where you were. Each fixture and gate of part B (11.1) and each gate of 11.2 and 11.3 must
   be made with a binary that has this correction. A fixture or gate run of part B or part C that was made with a
   binary without it: repeat it (for part B: before its commit, or as a second commit "FP2-A part B after correction
   1").

## 3. Not in this order

Any other change of the text. A change of the progress test of the volume `Split`. A change of the test `Blocked`.
