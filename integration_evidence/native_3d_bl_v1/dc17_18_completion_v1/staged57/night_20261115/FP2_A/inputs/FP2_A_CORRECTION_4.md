# FP2-A correction 4: the face alarm of correction 3 was wrong; K3 of build627 counts as ended

Author: reviewer, 2026-11-21 (checked against your evidence). For the main agent. The user sends it; it is a user order.
It changes ONE alarm of `FP2_A_CORRECTION_3.md` section 3 item 2. No production change (R2). All other parts of the text
and of corrections 1, 2 and 3 stay. Copy this file with its SHA256 into `R/night_20261115/FP2_A/inputs/`. R12 holds: do
not stop a job that runs.

## Why

Your stop was correct: the alarm of correction 3 fired (rank 4, pass 1, `faces_out` = 19,559 above 18,101). The alarm
itself was a fault of the reviewer. It took the same growth for each rank (84,088 / 48,954). That is not true: a rank
holds one part of the surface, and the distance splits are on the curved parts. In K3 of build627 ranks 0 and 1 made no
split, and ranks 4 and 5 made 7,145 and 7,609 splits. The sum is what counts: the `faces_out` of pass 1 over the six
ranks are 10,314 + 10,186 + 7,863 + 12,970 + 19,559 + 23,232 = 84,124, and LAB-S on one rank (build627) gave 84,102.
The largest star after the surface steps is 168 and `sweeps_distance` is at most 14. This is no runaway.

## 1. The change (test code and its evaluation only)

In correction 3 section 3 item 2 the alarm "for each rank: `faces_out` above 2 times its share" is removed.
In its place, one alarm for the run: the SUM over all ranks of `faces_out`, for pass 1 and for pass 2, above
1.5 times 84,102 (= 126,153): treat it as "did not end".
The other alarms stay as they are: `star_max` above 1,000 on a rank, `sweeps_distance` = 40 on a rank, and the 600 s of
correction 2 section 2 item 1.

## 2. What follows

1. Evaluate K1 and K3 of build627 again with this rule, from their receipts. No new run of K1, K2 or K3. K2 of
   build626 stays valid (surface off; correction 3 section 3 item 2).
   Write the new evaluation beside the old one (the old summary stays as history; R8: the first stop is not relabelled).
2. If both count as ended with no alarm: correction 2 section 2 item 1 holds. Continue the text: finish ALL of 11.2
   on build627 (the run CORE of 11.2 too), then 11.3 (runs S1 and S2), all on build627. Commit the new evaluation ("FP2-A rank check: K1 and K3
   ended, correction 4").
3. If you already stopped after 11.2 when you read this: verify that no owned job runs, then do what is left of 11.2
   (if the run CORE is not made yet: make it first), then 11.3.
4. If an alarm still fires with the new rule: the stop of correction 2 section 2 item 2 holds as before.
5. In the report of 11.3 write, for each rank, stage and pass of event 1 of S1, the row `NATIVE3D_LOCAL_SURFACE` and
   the sum of `faces_out` over the ranks (a report, not a gate).

## 3. Not in this order

A production change. A new run of the rank check. A change of a gate of 11.2 or 11.3.
