# FP2-A correction 2: a rank check on the lab input before the full runs with the surface option

Author: reviewer, 2026-11-21 (checked against the source). For the main agent. The user sends it; it is a user order.
It adds ONE gate to `FAST_PATH_STAGE2_A.md` ("the text") and changes no rule of it. Copy this file with its SHA256 into
`R/night_20261115/FP2_A/inputs/`. R12 holds: do not stop a job that runs. Test code only (R3); no production change (R2).

## Why

Your decision in part B was permitted by the text (fixture 7: "or build a small case"), and the small case passed. But
the attempt before it is a result that the reviewer must understand before the full runs: the LAB1 input with 2 ranks
printed no row of pass 1 in 5400 s (build622; the row comes after a vote of both ranks, so at least one rank did not
end pass 1), and the same pass needs 57.5 s on one rank. No loop of the core is without a limit and a pass has no
collective call, so inside the pass this is a very large slowdown, not a deadlock. One set of 116 process samples
(taken before 12:42, not kept) put the sampled rank in the quality work (`Star` called from `MovePoint` 88.8 %,
`EdgeRing` from `RemoveEdgePlan` 6.9 %). The full runs of
11.3 use 6 ranks and the same code, with 5400 s for an event. The cause is not known. The reviewer's first guesses (not
facts): points with a very large count of cells around them after the surface splits; `MovePoint` computes `Star`
before its test `Blocked` (Changes:454-459) and `Star` costs the square of that count; a border point keeps the mark
`unresolved` of the meeting (text 4.4), so cells near a rank border can stay `Blocked`. Correction 1 can change this
picture in both directions: it should remove most of the 1,163 long surface edges that were left, but step L without
the progress test can also make more and longer edges from a volume point to the new surface points.

## 1. The rank check (after correction 1 and its repeats; BEFORE the gates of 11.2)

With the binary that has correction 1. Input LAB1. The sequence of `RunLocalCore` (meeting, pass 1, exchange, shift
"lower", meeting, pass 2, exchange, shift "higher", then section 8 of the text), as your rank-2 lab did.
In the test code: `stats.report = true` and a `stats.sink` that prints one row for each sweep at once (flush): rank,
pass, kind, round, sweep, candidates, live cells, the counters of the sweep, seconds. In the same callback, and one
time after the surface steps (before `LengthWork`; if no hook exists there, at the first sweep row): the largest count
of cells around one point, the count of points with more than 100 and with more than 1,000 cells around them, and the
counts of `unresolved` points and of `Blocked` cells (from `mesh.Cells()` and `mesh.Points()`; test code).
The sink gets no pass and no rank: capture them in the lambda. `Blocked` is private: count the live cells with a
point that has `unresolved`. The count of cells around a point = the live cells that contain it. Add the test of
exact positivity of each owned cell (as `ReportExact` of LAB-S), and allow 6 ranks in the fixture and in its runner
(test code). Run K3 uses all 6 heavy processes of R1: no other job at that time.
Each run has a limit of 20 minutes (wrapper). One owned job at a time (R1).

| Run | Ranks | `surface_splits` |
|---|---|---|
| K1 | 2 | on |
| K2 | 2 | off |
| K3 | 6 | on |

Run K3 only if K1 ended inside its limit.
"Ended" = both passes and section 8 are complete on each rank, each cell is exactly positive, the check of 8.4 passes,
and the bytes of `Snapshot()` are equal on all ranks.
Write for each run: ended or not; for each rank and pass the row of section 9 of the text and the seconds of the
surface steps, the length work and the quality work; the star numbers above; for a run that did not end: the last
sweep row of each rank and the 5 sweep rows with the most seconds.

## 2. What follows

1. K1, K2 and K3 ended, and no pass of K3 needed more than 600 s on a rank (an alarm for a runaway, not a time gate):
   continue the text (11.2, then 11.3). Write the numbers in the report.
2. Else: the reviewer needs these numbers to write the fix. Commit the evidence ("FP2-A rank check: stopped before
   11.3"), then do the gates of 11.2 (they do not use the surface option; they prove the default), commit and push as
   the text says, and then STOP: do not start a run of 11.3. Verify that no owned job runs and wait for the user.
   Do not change production code for this (R2).
3. If K2 (surface off) did not end either: write it first in the progress file; the same rule 2 holds.

## 3. Not in this order

A production change. A change of a limit of the core. A change of the text 11.2 or 11.3 (except the stop of 2.2).
