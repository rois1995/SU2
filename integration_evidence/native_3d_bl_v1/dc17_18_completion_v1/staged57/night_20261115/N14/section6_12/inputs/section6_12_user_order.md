### 6.12 M25 result; N14 gets an exact condition (the lift of the split point) (2026-11-17, 10:30; this section wins over 6.10 item 3 where they differ; do not stop a running job)

Facts, from your evidence (`R/night_20261115/N13/section6_10/REPORT.md`, `M25_table.md`,
`cases/m25_split_20261117T075134_2706022/m25_split_outcomes.csv`; the reviewer's
read-only scripts are `m25_read.py` and `m25_lift.py` in
`/tmp/claude-1000/-media-rausa-4TB-SU2-Versions-SU2-NativeIntegrated/f3d988e5-f14f-4a96-b6fb-963ec11730e7/scratchpad/analysis/`):

- N13 is PASSED. Your prediction agrees with the first N11 verdict in 733 of 825 trials.
- Name: the LIFT of an edge = the distance from the chord midpoint to the bound point
  (your column `midpoint_distance`).
- In all 4,816 parent faces the lift is at or below the certified bound of the face
  (largest ratio 0.995). All 3,814 parent faces on an edge with lift > 1e-4 fail the
  tolerance (0 pass). So: while an edge with lift > tolerance is in the mesh, no wall
  face on it can pass the geometry gate. Its split is required.
- The first N11 verdict of the edges that are in a failing face, by the lift:

  | lift | accepted | rejected by the guidance check | median lift / tolerance of the rejected | rejected ones that your prediction calls an improvement |
  |---|---:|---:|---:|---:|
  | above the tolerance | 90 | 399 | 2.60 | 31 of 379 |
  | at or below the tolerance | 15 | 262 | 0.00 | 0 of 261 |

- For the 399: the smaller child has 0.34 of the parent bound (median), the larger
  child 1.13, and no child is above (parent bound + tolerance). Where the other old
  edges of a failing larger child are in the list (562 children), one of them has lift >
  tolerance in 562 of 562. So the reading of 6.10 is confirmed: the larger child fails
  because it keeps another edge that needs its own split. The sum check cannot see
  this progress.
- For the 262 the check is right (lift near zero: the edge lies along the direction
  with no curvature).
- The law estimate is 1.76 times the lift (median; quartiles 1.51 and 2.59; 5 % far
  above). 181 of the 1,922 edges that pass the law condition of 6.10 item 3 b have a
  lift at or below the tolerance, and 166 edges with lift > tolerance do not pass it.
  The lift is exact, it costs nothing (the trial has the bound point), and it needs no
  law call at the trial. So the lift replaces the law in the condition.

1. N14 = 6.10 item 3 with these changes. Parts a, e and f of 6.10 item 3 stay (B2 and
   the print-only changes of 6.9 come back by symbol; the gates; the report).

   b (replaces 6.10 item 3 b). Rule "required split". For the validation of the PRIMARY
   patch of an edge-split trial (the `validate_private` call for the result of
   `SplitEdge` or `InsertEdgeCavity`; source `primary` of 6.9), `geometric_change` is
   not called, in none of its call places inside that validation, and the trial
   continues as if the check passed, when ALL of these are true:
   - the `Adapt` call has a surface target (`surface_target`) and the patch is `curved`
     (the two conditions under which the check does work today);
   - the trial is not a coarsening and not a repair, and the choice comes from the
     geometry queue (`choice.geometric`);
   - `fresh` has a node with the identity of the split point (`point.id`);
   - lift > `options.surface_tolerance` (the tolerance that `GeometryGuidanceExcess`
     gets), strict. Lift = the Euclidean distance between `point.p` as the successful
     split call got it (the position on the chord) and the coordinates of that node in
     `fresh`. Compute the three differences and the sum of squares in `long double`
     and compare the squares.
   In each other case the guidance check runs as today. An interior edge has lift 0, so
   the rule is false for it. The joint path (`RefitWallSplit`, `ReconstructWallSplit`,
   `ReconstructSplitPatch`) keeps its checks: it runs only when the primary patch was
   not approved. Each validity guard stays (I part B1 item 2). For a required split
   `geometric_progress` is false ("not known"); source check as in 6.10: `metric_trial`
   reads it for the kind `GeometryRepair` only, and the kind here is `GeometryRefine`.
   The projection has a search tolerance of (tolerance / 8). No margin is added for
   it: the guidance check is a progress heuristic, not a validity guard, and the
   final geometry gate does not change.

   c (replaces 6.10 item 3 c). Readable code:
   - one pure function in the wall law file, with a comment that gives the reason of
     the rule in two lines, for example
     `bool SplitPointLiftRequiresSplit(const Point& chord_point, const Point& wall_point, double tolerance)`,
     and a unit case (lift below, equal, above; equal gives false);
   - the engine computes the bool ONE time, after the successful primary split call and
     before the primary `validate_private` call, and gives it to the validation as an
     explicit argument or as a local of the trial. No test of the rule inside the joint
     or repair paths;
   - no law call at the trial, no search for a wall face;
   - no new field in `CellState`, in a `Choice` row that travels, in `Session`, or in the
     timer enum. Each `NATIVE3D_TIMER_SUMMARY` row keeps `fixedstorage=4120`.

   d (replaces 6.10 item 3 d). Counters, print only, in the wall counters that exist:
   `wall_splits_required` (trials, accepted); `wall_splits_checked` (geometry queue,
   lift at or below the tolerance: trials, rejected by the guidance check, accepted);
   and for the new wall faces of the required splits, when `Selection::Add` measures
   them: count, count at or below the tolerance, count above (bound of the parent
   face + tolerance). The last count is 0 in M25. A value above 0 is a report (R4), not
   a gate.

2. Release. N14 starts only after the user order with the words "N14 released" (6.10
   item 3). When that order is there:
   - When: at the next boundary of the cut (after the sub-step in work has passed its
     gates and is committed), as N13.
   - N15 (6.11) goes into the same build. The full two-event run of N14 (I "Gates of B1
     and B2" item 1) gives the M26 numbers. No other full run.
   - Purpose rule of 6.10 item 3 e: no change (at least 689 accepted splits at the exit
     in BOTH G3 runs).
   - If N14 passes: its G1, G2, GQ and G3 runs are the references of the cut from then
     on (G1: list each changed case of the wall stage with the old and the new numbers;
     G2 and GQ: the rule of 6.8 item 2 for the `census` calls, with the smaller count of
     `NATIVE3D_WALL_LAW` lines of 6.9 item 2 a). No extra reference run.
   - If N14 fails a result gate: R5. Revert N14 in one commit; the cut goes on with
     the old references. For N15 then: if the full run of this build was done and its
     no-BL results are identical, its M26 numbers are valid and the N15 lines stay in.
     If the full run was not done: revert N15 with N14 and use the second case of
     6.11 item 1 (one commit before the G5 run at the end of phase C).

3. The morning report (N7): one row for N13 (PASSED, the reading of 6.10 confirmed, the
   condition changed from the law to the lift), one for N14.
