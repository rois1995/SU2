# Latest user orders, 2026-11-17: N14 RELEASED and planned phase-B stop

User order, 2026-11-17 (LATEST): planned stop at the end of phase B of the cut. When phase B has passed its end gates (G3, G4, GQ) and is committed and pushed: update progress.md and HANDOFF_Codex.md, verify that no owned job runs, write the terminal handles, then STOP and wait for the user. Do not start phase C. This order wins over "never idle" for this one point only. Until then, work as ordered (N14 with N15, then the rest of phase B). If phase B cannot be completed under R5: stop at the same point, after the revert is committed and pushed.

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
   - CORRECTED 2026-11-17, 10:10 (one row was missing; an error of the reviewer). If
     N14 passes: N14 changes the choice of the edge. So the row "the first 336 edits
     identical to the P6c reference" of gate G3 (6.6 item 1 a) ends for all later G3
     checks of the cut. The row "at least 336 splits" and each other row of 6.6 item
     1 a stay. The identity part (6.6 item 1 b) uses the first G3 run of N14 as its
     reference run. If N14 is reverted: no change of gate G3.
   - If N14 fails a result gate: R5. Revert N14 in one commit; the cut goes on with
     the old references. For N15 then: if the full run of this build was done and its
     no-BL results are identical, its M26 numbers are valid and the N15 lines stay in.
     If the full run was not done: revert N15 with N14 and use the second case of
     6.11 item 1 (one commit before the G5 run at the end of phase C).

3. The morning report (N7): one row for N13 (PASSED, the reading of 6.10 confirmed, the
   condition changed from the law to the lift), one for N14.

### 6.11 One more print-only measurement: M26, the wall lane census (2026-11-17, 08:00; user decision of 2026-11-17)

Purpose: the user accepted the DEFINITION of design part 8.5ab (D section 5.12,
revision 2.3; copy D again with its SHA256). Do not implement 8.5ab. The reviewer
needs two numbers for its text: how many wall commits a rank can make alone.

Item N15. Print only, under `options.timings`, in the census of N12
(`CNativeLaneCensus3D.hpp` and its call places).

1. When, to add no full run:
   - if N14 is released before the end of phase C of the cut: in the same build as N14;
   - if not: as one commit before the G5 full run of the end of phase C. That run gives
     the numbers. If that G5 run fails and the one diagnosis pass does not name another
     part: revert N15 first and repeat G5 one time.
2. Population: the commits of the class local + wall of N12, quality stage only.
3. For each such commit, from data that the commit place has (`old`, `fresh`, the
   edits of `BoundaryParameterState::Changes` for this proposal, the coverage delta,
   the ladder kind). Let e = the count of erased wall points and i = the count of
   inserted wall points of these edits:
   a. `link_local`: each wall face on each wall vertex of `old`
      (`BoundaryParameterState::FacesOnVertex`) is a face of a cell of this rank;
   b. the class:
      - S = `SameSkin` true, no parameter edit, empty coverage delta;
      - W1 = not S, e <= 1 and i <= 1 (one wall point added, moved or removed, or a
        wall reconnection with no point change);
      - W2M = not S, e = i >= 2, and `old` and `fresh` have the same count of wall
        faces (wall points moved only);
      - W2O = each other commit;
   c. for class W1 with i = 1: `first_proof` = the exact orientation of the inserted
      point against each wall face of `old` that is not a wall face of `fresh` is not
      negative (the sign rule of your M6b code).
4. One line for each rank at the stage exit:
   `NATIVE3D_WALL_LANE_CENSUS rank= commits= class_s= class_w1= class_w2m= class_w2o= w1_kind0= w1_kind1= w1_kind2= w2m_kind1= w1_link_local= w2m_link_local= w1_first_proof= w1_link_local_first_proof= seconds_s= seconds_w1= seconds_w2m= seconds_w2o=`
   (`seconds` = the busy seconds of the slots of these commits, the clock of N12).
   `commits` must be equal to the `commits` of the N12 row local + wall, and the four
   classes must sum to it.
4a. One more number, for the cost of the join of 8.5ab. In `Directory::PrepareParameters`,
   for the call `parameters->Prepare` on the union of the edits of all ranks: the count
   of calls, the seconds (a local clock, only under `options.timings`; no new timer
   operation), the count of edits and the count of faces. One line for each rank at
   the stage exit: `NATIVE3D_WALL_PREPARE rank= stage= calls= seconds= edits= faces=`.
5. No decision reads these values. No new timer operation, no new field in `Session`,
   `CellState` or a row that travels (each `NATIVE3D_TIMER_SUMMARY` row keeps
   `fixedstorage=4120`).
6. Gates: identical results. G1 by identity; G2 and GQ with all old integer rows
   equal; the six files of the full run of item 1 equal to the references.
7. Report: one table for each event (6 ranks), and for the sums: the share of S, of W1
   with `link_local`, of W1 with `link_local` and `first_proof`, and of W2M with
   `link_local`, in commits and in seconds; and the rows of item 4a.



## Earlier user orders (superseded where above differs)

User order, 2026-11-17 (LATEST): NIGHT_WORK_ORDER.md sections 6.10 and 6.11 govern from now. N10 withdrawn; N13 test-only M25 at the next cut boundary; N14 prepared and NOT released; N15 print-only M26 at the specified phase-C/N14 boundary. Do not implement 8.5ab. Do not stop a running job.

### 6.10 N11 result, the cause from the trial records, one measurement (N13), one prepared fix (N14, NOT released) (2026-11-17, 07:40; do not stop a running job; the cut goes on)

Facts, from your evidence (`R/night_20261115/N11/REPORT.md`, `R5_diagnosis.json`,
`reports/G3_all_wall_rows.csv`, and the trial records
`R/night_20261115/N11/validation/cases/section6_2_bl_20261116T221115_1302882/nose_target_observation.jsonl`):

- N11 failed its purpose rule: 1,046 / 1,301 = 0.804. Your code did what the text
  asks, and your revert was correct. The reading of the reviewer in 6.6 item 3 ("the
  choice of the edge is the cause of the rejections") was wrong. It was an error of the
  reviewer.
- The counter of 6.9 item 2 b: `ReconstructSplitPatch` gave 384 valid patches, the
  guidance check rejected all 384. The three joint operators used 53 s of the 388.8 s
  and gave 0 accepted splits.
- The reviewer joined the trial records with the strict law tables of the `saved492`
  fixture (the first record of each edge of the entry surface that has a failing wall
  face; 766 edges):

  | The edge has the largest law estimate in | accepted | rejected by the guidance check | accepted share | median estimate / tolerance |
  |---|---:|---:|---:|---:|
  | its two wall faces | 57 | 205 | 22 % | 5.0 to 5.9 |
  | one of its two wall faces | 37 | 76 | 33 % | 2.3 to 5.1 |
  | none of its two wall faces | 11 | 380 | 3 % | 1.7 |

  750 of the 766 edges have the `need` flag. So the flag alone does not select.
- Reading of the reviewer (N13 tests it before a production change):
  1. The guidance check asks that ONE split makes the sum of the squared excess
     smaller. A wall face on a surface that curves in one direction has usually two
     edges that cross the curvature. A split of one of them leaves one child face with
     the other edge. That child keeps the old error, so the sum does not go down, and
     the check rejects the split. But the split is necessary: a face cannot pass while
     it has an edge whose midpoint is farther from the surface than the tolerance.
  2. After this rejection the engine tries the other edges of the same face. These
     trials are half of all rejected trials, and 97 % of them are rejected.
  So B2 alone cannot pass (the veto stays), and B1 with the text of 6.6 item 5 would
  accept splits of the wrong edges (367 of the 380 edges of the last row have the
  `need` flag). The two parts must come together, with a sharper condition (item 3).
- M24 (your `reports/M24_full534.csv`): in the quality stage the busy seconds are 71 %
  / 83 % (event 1 / 2) in the class local + wall, 26 % / 12 % in shared + wall, and
  2.5 % / 4.2 % in local + interior. A wall trial costs 79 ms, an interior trial 14 ms.
  So step 8.5a as written (local + interior only) is not worth a task. No action.

1. N10 with the text of 6.6 item 5 is WITHDRAWN. Do not implement it.

2. Item N13 = measurement M25, test code only. No production path changes.
   When: at the next boundary of the cut (after the sub-step in work has passed its
   gates and is committed). Then the cut goes on.
   - Copy with its SHA256 into your evidence folder:
     `/tmp/claude-1000/-media-rausa-4TB-SU2-Versions-SU2-NativeIntegrated/f3d988e5-f14f-4a96-b6fb-963ec11730e7/scratchpad/analysis/m25_edges.csv`
     (SHA256 5d3a73801727bb3443b7557d6afe56434a6ddc3dc89577aa6825b27892d7accf;
     2,408 edges of the `saved492` entry surface; columns `edge_a`, `edge_b`, `law_pick`,
     `n11_first_verdict`, `failing_faces`, `limiting_faces`, `limiting_failing_faces`,
     `need`, `estimate`).
   - Where: one new hidden unit case beside the case that wrote `d9_geometry_edges.csv`
     and `d9_geometry_faces.csv` for `saved492`, with the same setup (the same saved
     state, parameter state, atlas budget, tolerance 1e-4).
   - For each edge of the list, in the order of the file:
     a. the production call `PredictGeometrySplit(edge, endpoints, geometry, tolerance)`
        (`CNativeBoundaryParameters3D.cpp`): `complete`, `before`, `after`,
        `evaluated_faces`, seconds, atlas work;
     b. in test code, with the same calls that the body of that function makes
        (`PrivateParameters`, `BindNewPoint`, `ContainsFace`, `SurfaceDeviation` with
        `Magnitude`): the bound of each of the two parent faces and of each of the four
        child faces, the bound point, the distance from the chord midpoint to the bound
        point, the length of the edge;
     c. `EstimateWallEdge` for the edge and for the four other edges of the two faces
        (anchor = the facet of the face).
   - Self check of your test code: the sums of the squared excess of b, added in the
     order of the function, are bit-equal to `before` and `after` of a. If not: R3.
   - If a call of b is not reachable from test code: do not change production. Write
     the columns that you can and name the missing ones (print-only rule of R4).
   - Output: `m25_split_outcomes.csv` (one row for each edge, the columns of the list
     first) and `M25_table.md` with, for each group (`limiting_failing_faces` 2 / 1 / 0,
     and `n11_first_verdict`): count; count complete; share with
     `after < before * (1 - 1e-12)`; for each parent face that has the edge as its
     largest-estimate edge: the median of (smaller child bound / parent bound) and of
     (larger child bound / parent bound); the share of the children with a bound at or
     below the tolerance; the share of the children with a bound above (parent bound +
     tolerance). And one 2 x 2 table: prediction of a against `n11_first_verdict`.
   - Limits: one process, nice 19, at most 60 minutes of run time. If the estimate
     after 100 edges is above 60 minutes: do the rows with `n11_first_verdict` not
     `none` first, then the others until the limit, and report the count.
   - Gates: the test build, the case ends, the self check. G1 to G5 do not run for
     this item (no production change; a hidden case is not in the G1 list).

3. Item N14 = B2 and a limited B1 as ONE connected item. It is PREPARED, NOT RELEASED.
   Start it only after a user order that has the words "N14 released". That order can
   change the numbers below after the reviewer has read `M25_table.md`.
   a. Put back the production and test paths of `5488d1f1b5` (B2 and the print-only
      changes of 6.9: no entry census, `NATIVE3D_WALL_SOURCE`, the law counters) on
      the head of that time, by symbol (the cut has moved text).
   b. New rule, "law-required split". In the validation of the PRIMARY patch of an
      edge-split trial (the `validate_private` call for the result of `SplitEdge` or
      `InsertEdgeCavity`; source `primary` of 6.9), `geometric_change` is not called
      and the trial continues as if it passed, when ALL of these are true:
      - the `Adapt` call has a surface target; the trial is not a coarsening and not a
        repair; the choice comes from the geometry queue (`choice.geometric`);
      - W is a wall face of `old` that contains the edge and is a face of the cell of
        the queue entry (`choice.priority_cell`; source check: if this field is not
        that cell, write it in the list and use "a wall face of `old` that contains
        the edge");
      - the law of A1 is supported for the three edges of W (anchor = the facet of W);
      - the estimate of the edge is the largest of the three (equal is permitted);
      - `EstimateSaysNeed` is true for the edge.
      In each other case the guidance check runs as today. The joint path keeps its
      checks (I part B1 item 3). Each validity guard stays (I part B1 item 2).
      For such a trial `geometric_progress` is false ("not known"). Source check:
      `metric_trial` uses it for the kind `GeometryRepair` only, and the kind of this
      trial is `GeometryRefine`.
   c. Readable code: one pure function in the wall law file (three estimates and tau
      in, one bool out; with a unit case) and one small helper in the engine that
      finds W. No new field in `CellState`, `Choice` rows that travel, `Session` or
      the timer enum (the modeled bytes must not change; see 6.8).
   d. Counters, print only: `wall_splits_law_required` (trials, accepted); for the
      new wall faces of these splits, when `Selection::Add` measures them: count, count
      at or below the tolerance, count above (bound of the parent face + tolerance).
   e. Gates: I "Gates of B1 and B2" items 1, 2, 4 and 5; G3 two times with the
      validity rows of 6.9 item 1 and the same edits up to the smaller accepted count.
      Purpose: in BOTH runs the accepted splits at the exit are at least 689
      (1.5 x 459; references without the entry census: 457 in N9, 459 in N11; reason
      of the number: the guidance check and the joint operators use about 230 s of the
      389 s today). If not: R5, cause in the proposal, revert in one commit, keep the
      numbers, go on.
   f. Report: the list of 6.6 item 4, the rows of d, `NATIVE3D_WALL_FRAME`,
      `NATIVE3D_WALL_SOURCE`, `guidance_seconds`.

4. Order from now: the cut as planned (6.3), with N13 at its next boundary. N14 only
   after its release. Then N6, N7. The morning report (N7) gets one row for N11
   (FAILED, reviewer reading wrong), one for N10 (withdrawn), one for N13.

### 6.11 One more print-only measurement: M26, the wall lane census (2026-11-17, 08:00; user decision of 2026-11-17)

Purpose: the user accepted the DEFINITION of design part 8.5ab (D section 5.12,
revision 2.3; copy D again with its SHA256). Do not implement 8.5ab. The reviewer
needs two numbers for its text: how many wall commits a rank can make alone.

Item N15. Print only, under `options.timings`, in the census of N12
(`CNativeLaneCensus3D.hpp` and its call places).

1. When, to add no full run:
   - if N14 is released before the end of phase C of the cut: in the same build as N14;
   - if not: as one commit before the G5 full run of the end of phase C. That run gives
     the numbers. If that G5 run fails and the one diagnosis pass does not name another
     part: revert N15 first and repeat G5 one time.
2. Population: the commits of the class local + wall of N12, quality stage only.
3. For each such commit, from data that the commit place has (`old`, `fresh`, the
   parameter edits, the coverage delta, the ladder kind):
   a. `link_local`: each wall face on each wall vertex of `old`
      (`BoundaryParameterState::FacesOnVertex`) is a face of a cell of this rank;
   b. the class: S = `SameSkin` true, no parameter edit, empty coverage delta;
      W1 = not S, and ONE point is in each new wall face (a wall face of `fresh` whose
      key is not a wall face of `old`); W2 = each other commit;
   c. for class W1: `first_proof` = the exact orientation of that point against each
      old wall face of `old` that is not in `fresh` is not negative (the sign rule of
      your M6b code).
4. One line for each rank at the stage exit:
   `NATIVE3D_WALL_LANE_CENSUS rank= commits= class_s= class_w1= class_w2= w1_kind0= w1_kind1= w1_kind2= w1_link_local= w1_first_proof= w1_link_local_first_proof= seconds_s= seconds_w1= seconds_w2=`
   (`seconds` = the busy seconds of the slots of these commits, the clock of N12).
   `commits` must be equal to the `commits` of the N12 row local + wall, and the three
   classes must sum to it.
5. No decision reads these values. No new timer operation, no new field in `Session`,
   `CellState` or a row that travels (each `NATIVE3D_TIMER_SUMMARY` row keeps
   `fixedstorage=4120`).
6. Gates: identical results. G1 by identity; G2 and GQ with all old integer rows
   equal; the six files of the full run of item 1 equal to the references.
7. Report: one table for each event (6 ranks), and for the sums: the share of S, of W1
   with `link_local`, and of W1 with `link_local` and `first_proof`, in commits and in
   seconds.


User order, 2026-11-16 (LATEST): NIGHT_WORK_ORDER.md section6.9 governs N11/N10; read before their production edits. The completed6.8 campaign remains preserved with its original pinned policy.

### 6.9 Additions for N11 and N10 (2026-11-16, 20:45; read before the first edit of N11; do not stop a running job)

Facts, from the G3 run of the group of 6.8
(`R/night_20261115/N8/A2_A3_N12/section6_8/validation/cases/section6_2_bl_20261116T181040_727116/run.log`):

- `NATIVE3D_WALL_LAW boundary=entry`: 73.6 s. It runs inside the clock of the wall
  stage (388.8 s). So the stage accepted 373 splits, not 457 as in the N9 run. The
  census costs about 0.5 ms for each edge incidence (the same in the full run: 10 to 16
  s for each rank). The reviewer did not estimate this cost. It was an error.
- `NATIVE3D_WALL_FRAME`: `RefitWallSplit` 408 calls, 0 valid, 9.3 s;
  `ReconstructWallSplit` 396 calls, 0 valid, 18.5 s; `ReconstructSplitPatch` 398 calls,
  380 valid, 26.7 s. The 380 valid results go to the guidance check a second time.
  How many of them are approved is not known.
- Error in the gate text of 6.6 items 4 and 5: "validity as in rule 1 (a)" has the two
  rows "at least 336 splits" and "the first 336 edits identical to P6c". N11 and N10
  change the choice of the edge and the approval. So these two rows cannot hold for
  them.

1. G3 validity for N11 and for N10 (replaces "validity as in rule 1 (a)" in 6.6 items 4
   and 5): all private cells positive with sensors; zero upper refinements; the modeled
   storage and the job peak inside their limits; no validity failure. The rows "at
   least 336 splits" and "the first 336 edits identical to P6c" are NOT gates of N11 and
   N10. The other rows of 6.6 items 4 and 5 stay: two runs, the same edits up to the
   smaller accepted count, the purpose rule of each item.
   The split count of N11 is a report. Reference for the report: the two numbers 457
   (N9 run, no census) and 373 (group run, with the entry census).

2. Print-only changes, in the SAME build as B2 (no extra build, no extra run):
   a. Remove the `entry` call of `PrintWallLawCensus`. The `exit` call stays. The rule
      of 6.8 item 2 for the `census` calls stays; the count of `NATIVE3D_WALL_LAW`
      lines is smaller now.
   b. For the edge-split trials of an `Adapt` call with a surface target: count the
      results of `validate_private` by the source of the patch. Four sources: `primary`
      (the result of `SplitEdge` or `InsertEdgeCavity`), `RefitWallSplit`,
      `ReconstructWallSplit`, `ReconstructSplitPatch`. Line at the stage exit:
      `NATIVE3D_WALL_SOURCE rank= source= validated= guidance_rejected= approved=`.
      No decision reads it. Report the sums beside `guidance_trials`,
      `guidance_rejected` and `accepted`. A difference of the sums is a print-only
      contradiction (R4): write it, continue.

3. N11 purpose rule: no change (geometry queue, guidance rejections / guidance trials
   below 0.70 in both runs). References: 0.82 (N9: 1,170 / 1,425) and 0.83 (group run:
   974 / 1,169). These ratios count a choice two times when `ReconstructSplitPatch`
   gives a valid result. So report also the ratio of the source `primary` alone.

4. No change of a decision for the three joint operators in N11 or N10. The reviewer
   writes a text for them after the numbers of item 2 b.


User order, 2026-11-16 (LATEST): NIGHT_WORK_ORDER.md section6.8; wins over I part A where different. Do not stop a running job.

### 6.8 Correction of 2026-11-16, 17:00: N8 A1 again, in the group of A2 / A3 / N12 (two reviewer errors; this section wins over I part A where they differ)

Facts, from your evidence (`R/night_20261115/N8/A1/REPORT.md`, `R5_diagnosis.json`,
`source_check.md` risks 7 and 8):

- Error 1 of the reviewer. The check "the same edge CSV hashes" cannot hold. The law
  moves from a test file (compiled with fast-math) into the list of the native
  adaptation sources (`accurate_sum_lib`, strict arithmetic). That place is correct:
  a law that a decision reads must have strict arithmetic. So the last digits of the
  scores change (49,392 scores, largest relative difference 3.96e-8), and the
  limiting side changes where two sides have the same score inside that difference
  (298 / 296 rows). The rows, the `need` flag of each of the 82,559 edges and all other
  columns are equal. Your code did what the text asks. Your revert followed the written
  check; it was correct to do it.
- Error 2 of the reviewer. I A1 item 5 asks a new timer operation `wall_law`. A new
  `Operation` makes `sizeof(Session)` larger (192 B). `Session::ReservedBytes()` goes
  into `caller_bytes` (`CNativeEngine3D.cpp` 1355, `CNativeRemesher3D.cpp` 212), so each
  modeled byte row and the storage admission would change. This is not "identical
  results". G2 would have failed for this cause.

1. When. Do not stop a running job. Let the check that runs now end. Then, before the
   next check of the group A2 / A3 / N12, put A1 into the same group (items 2 to 4).
   The checks of the group that passed on build 533 run again on the new build; the
   join still saves one full set of gates and one full run.
   Exception: if the full two-event run of the group is live or done when you read
   this, finish and judge the group as written. Then do A1 alone with items 2 to 5.

2. Put back the production and test paths of `eb6d94042c` on the current head, with ONE
   change:
   - no `Operation::WallLaw` and no `wall_law` name. `PrintWallLawCensus` opens
     `Scope(Operation::Census)` (the operation that `observe_progress` uses already).
     `sizeof(Session)` must not change: each `NATIVE3D_TIMER_SUMMARY` row prints
     `fixedstorage=4120`, as the reference does.
   - Consequence for your comparators, an exact rule and not a tolerance: for each rank
     and each timer boundary, the sum of `calls` over the rows of the operation
     `census` is (reference value) + (count of `NATIVE3D_WALL_LAW` lines that this rank
     printed before that boundary). Each other old row stays strict: integers, modeled
     bytes, calls.

3. The check of A1 on the three N5 geometry fixtures (replaces "the same edge CSV
   hashes" of I "Checks of A1"). Against the R1-repeat receipt of each fixture:
   (a) the same rows in the same order (`edge_a`, `edge_b`);
   (b) in each row, identical: `tau`, `need_threshold`, `need`, `supported_sides_a`,
       `supported_sides_b`, `unsupported_sides_a`, `unsupported_sides_b`;
   (c) in each row: |`score_max` - reference| <= 1e-6 x max(reference,
       `need_threshold`);
   (d) the same M1 integer rows (7,521 / 6,759 / 4,368 need edges);
   (e) report only: the count of rows with another `limiting_side_a` or
       `limiting_side_b`, and the largest relative score difference.
   The edge CSV files of this run are the law reference from now (strict arithmetic).

4. One more print-only change in the same build (for the reviewer's text on the wall
   refusals). The N9 table has 345 times one text, 15 texts that differ only by a facet
   and a cell number, and 544 in `more`. So: before the lookup in the 16-entry table,
   replace each run of decimal digits in the stored 96 characters by one `#`. Nothing
   else changes. The sum of the counts must be the count of operator refusals.
   And, print only, in `CNativeWallDiagnostics3D.hpp` (`ObserveGuard`): the class
   `other_geometry` has three more classes beside it, by the operation name that
   `guarded_geometry` gets already: `RefitWallSplit`, `ReconstructWallSplit`,
   `ReconstructSplitPatch`. For each of these three: the count of calls, the count of
   refusals for each reason (the `NATIVE3D_WALL_REFUSAL` rows that exist), and the
   seconds of the call (one clock around the call in the frame loop, only when the
   counters are enabled). New line at the stage exit:
   `NATIVE3D_WALL_FRAME rank= operation= calls= valid= seconds=`.
   The refusals of the three new classes and of the rest of `other_geometry` must sum
   to the old `other_geometry` rows. No decision reads these counters.
   Reason (reviewer's source reading, for the report): `SplitEdge` and
   `InsertEdgeCavity` have 0 refusals in the N9 run. All 904 refusals come from these
   three operators. The engine calls them only after the first split of a choice of
   the geometry queue was not approved. So they follow the guidance rejections.

5. Gates of the group (A1 + A2 + A3 + N12 + item 4), one time: build; the unit cases
   of the items; item 3; M6b; G1 by identity; G2 and GQ with all old integer rows equal
   (rule of item 2 for the `census` calls); G3 by rule 1 of 6.6; G4; the safeguards;
   ONE full two-event run with its six files equal to the references. Report the
   `NATIVE3D_WALL_LAW` rows with their seconds, and the new reason table of the G3 run.

6. If a result gate of the group fails: R5. The one diagnosis pass names the part from
   the first number that differs (A1, item 4, or A2 / A3 / N12). Revert that part only
   in one commit, build, repeat the failed check one time, and go on with the group.
   If the pass cannot name the part: revert A1 and item 4, and go on.

7. After the group has passed: N11 (B2), then N10, as items 4 and 5 of 6.6 say. Then
   6.3, N6, N7.


User order, 2026-11-16 (LATEST addition): section6.7, N12/M24 print-only with N8 A2/A3; do not implement8.5.

### 6.7 One more print-only measurement: M24, the lane census (2026-11-16, 15:00)

Purpose: design step 8.5 (not accepted, do not implement it). The reviewer needs one
number to write its text: which share of the quality work a rank could do alone.
Text: D section 5.11 and row M24 of D section 9 (D revision 2.2; copy it with its
SHA256).

Item N12. Do it in ONE build with N8 parts A2 and A3 (all three are print only), with
the same gates and the same full two-event run. No extra run.

1. Where: the private loop of a round, for each slot that reaches `evaluate_candidate`,
   in the quality stage and in the construction stage. Under `options.timings` only.
2. Class of the slot, from data that the slot has already:
   - `local` = each `dependencies.cells[i].owner` is this rank;
   - `wall` = one or more `imported.cells[i].physical` facets are bound.
   Four classes: local / shared x interior / wall.
3. For each class and stage: trials, approved patches, commits, busy seconds of the
   slot (the clock that the private loop has), optimizer work.
4. For the commits of each class: the count with `SameSkin(old, fresh)` true, the count
   with no parameter edit, the count with an empty coverage delta. Compute `SameSkin`
   for this only where the code does not compute it already; its result must not be
   read by a decision.
5. Print at the stage exit, one line for each rank, stage and class:
   `NATIVE3D_LANE_CENSUS rank= stage= class= trials= approved= commits= seconds= work= same_skin= no_parameter_edits= no_coverage=`.
   The sums over the classes must agree with the ladder counters that exist.
6. Gates: identical results. G1, G2 and GQ with all old integer rows equal, G3 by rule
   1 of 6.6, G4, and the full run of N8 with its six files equal to the references.
7. Report: one table for each event (6 ranks x 2 stages x 4 classes), and the share of
   the busy seconds and of the commits in the class local + interior.


User order, 2026-11-16 (LATEST): NIGHT_WORK_ORDER.md section6.6, wins over6.5/I/C where different. Do not stop a running job.

### 6.6 Correction of 2026-11-16, 14:50 (reviewer errors; this section wins over 6.5, over I and over C where they differ)

Facts, from your evidence (`R/night_20261115/N9/R5_diagnosis.json`, `REPORT.md`,
`B0_table.md`):

- The G3 fixture has two clock inputs. The observer before `PrepareSurface` runs for
  30 s and copies its work into the shared atlas budget
  (`UnitTests/SU2_CFD/adaptation/fixtures/native_actual_surface_target_observation.inc:18`),
  and the stage has 388.8 s. Three runs with identical decisions gave 473, 486 and 459
  splits. So the gate "all old integer rows of G3 equal" cannot hold. This gate text was
  an error of the reviewer. N9 did what its text asks: 459 common edits identical, entry
  work minus observer work 5,113 in both runs, counter sums agree. Your revert followed
  the written gate; it was correct to do it.
- N9 table: the guidance check rejected 1,180 of 1,437 trials (82 %) for choices of the
  geometry queue and 10 of 217 (5 %) for choices of the size queue. Cause `incomplete`
  1,187 times: `GeometryGuidanceExcess` stops at the limit `before`, that is, the new
  wall faces have more summed excess than the old ones. Cause `no_progress` 3 times.
- 865 of the 912 operator refusals have the label `other_geometry` and the class
  `other`. So their cause is still not known.
- 3,508 of 4,737 selected choices are `deferred` before a trial.

1. Gate G3, for all items and for C, from now:
   (a) Physical part, as before: all private cells positive with sensors, zero upper
       refinements, the modeled storage and the job peak inside their limits, at least
       336 splits, the first 336 edits identical to the P6c reference.
   (b) Identity part, for an item with "identical results": the accepted edits (edge and
       point of each edit) are identical to those of the G3 reference run, up to the
       smaller accepted count of the two runs; and `entry atlas_work` minus the observer
       work is equal in the two runs.
   (c) All other integer rows of G3 (entry `atlas_work`, rounds, split count, exit
       counters) are a report beside the reference. They are not a gate.
   The G3 reference run is the C492 run of section 6.1 (486 splits). G1, G2, GQ, G4 and
   G5 do not change. If you find a clock input in one of them, report it; do not change
   the gate.

2. N9 again, before the next production edit of another item.
   - Put back the three production paths of `bd709cb4c1` on the newest accepted commit.
   - One addition, print only: for each guarded trial that an OPERATOR refuses (origin
     not `Guidance`), count the distinct texts of `result.reason`. A fixed array of 16
     entries (the first 96 characters of the text, and a count) and one count `more`.
     Print at the stage exit: `NATIVE3D_WALL_REASON text="..." count=`. No decision reads
     it. The sum of the counts must be the count of operator refusals (912 in the table).
   - Gates: build; G3 by rule 1 (one run); G2. Report the B0 table again with the new
     rows. Then N9 is PASSED.

3. New order: N9 (item 2) -> N8 part A1 only (the law) -> N11 (B2) -> N10 (B1, with the
   new text of item 5) -> N8 parts A2 and A3 -> 6.3 -> N6 -> N7.
   - The G1 reference that you record now on the accepted base stays the G1 reference
     for these items.
   - If a production edit of B1 with the OLD text exists when you read this: do not
     judge it. Let a live job end, keep its numbers as "B1, old text, not judged", put
     the production paths back to the accepted base in one commit, and follow this order.
   Reason of the order: the 82 % against 5 % (facts above). Reading of the reviewer (I,
   N11 tests it): the geometry queue gives the score of a failing face to its edges by
   their length. So it splits the longest edge, also when the curvature is along another
   edge. The two new faces then keep the error, the summed excess is twice as large, and
   the guidance check rejects the split. For such a split the check is right. B1 with
   the old text would accept these splits. So B2 comes first, with the check in place.

4. N11 (B2): the text of I part B2 does not change. Its gates: I "Gates of B1 and B2"
   items 1, 2, 4 and 5 as written; item 3 is replaced by:
   - the G3 fixture two times; validity as in rule 1 (a); the two runs have the same
     edits up to the smaller of their accepted counts;
   - purpose: in BOTH runs, for the choices of the geometry queue, (rejections of the
     guidance check) / (guidance trials) is below 0.70. The reference is 0.82
     (1,180 / 1,437; two older runs differ by 0.003 for all queues together). If it is
     not below 0.70: the reading of item 3 is wrong. Keep the numbers, revert B2 in one
     commit (R5, cause in the proposal), do NOT do N10, and go on with N8 A2 / A3;
   - report beside the N9 table: all B0 rows, `law_weighted_faces`,
     `law_fallback_faces`, accepted splits for each queue, rounds, measured cells, exit
     `geometry_bad`, `size_bad`, `geometry_debt`, `size_debt`. The exit rows depend on
     the clock; they are a report.

5. N10 (B1), new text of item 1 of I part B1. `geometric_change` is not called when all
   of these are true: the `Adapt` call has a surface target; the trial is an edge split;
   it is not a coarsening and not a repair; and
   - the edge comes from the size queue, or
   - the edge comes from the geometry queue, the estimate of A1 is supported for this
     edge, and `EstimateSaysNeed` is true for it.
   In each other case the guidance check runs as today. Items 2 to 5 of I part B1 stay.
   Counters: `wall_splits_without_guidance` for each of the two queues, and
   `wall_splits_with_guidance`.
   Gates of N10: I "Gates of B1 and B2" items 1, 2, 4 and 5 as written; item 3 is:
   - the G3 fixture two times; validity as in rule 1 (a); the same edits up to the
     smaller count;
   - purpose: in BOTH runs the accepted count at the exit is at least 1.10 times the
     larger accepted count of the two N11 runs. If not: keep the numbers, revert B1 in
     one commit (B2 stays), go on;
   - report: the list of item 4, `guidance_seconds`, `upper_seconds`, and for the splits
     without the guidance check: how many of their new wall faces fail the exact
     measure in `Selection::Add`.
   The G3 runs of the last passed item are the G3 reference for the items after it.

6. The gate texts that this section replaces: I part A "Gates of part A" (G3 by rule
   1); I part B0 "Gate" (G3 by rule 1, G2); I "Gates of B1 and B2" item 3 (items 4 and 5
   above); C section 2, row G3 (rule 1). The old purpose rule "`geometry_bad + size_bad`
   below 7,984" is withdrawn: the exit counts depend on the clock, and a split of one
   failing face can give two smaller failing faces before the error is inside the
   tolerance.

7. Not a task now, for the report only: the 3,508 `deferred` choices. The reviewer
   writes a text for them after the N9 reason counts and the N11 numbers.


User order, 2026-11-16 (LATEST): section 6.5 and R1 clarification

### 6.5 Design steps 8.3 and 8.4 (first part), accepted by the user on 2026-11-16

The user accepted design revision 2 with steps 8.3, 8.4 and 8.4a. Copy these two files
with their SHA256 into your evidence folder and read them completely:

- `/tmp/claude-1000/-media-rausa-4TB-SU2-Versions-SU2-NativeIntegrated/f3d988e5-f14f-4a96-b6fb-963ec11730e7/scratchpad/IMPLEMENTATION_8_3_8_4.md` (I)
- `/tmp/claude-1000/-media-rausa-4TB-SU2-Versions-SU2-NativeIntegrated/f3d988e5-f14f-4a96-b6fb-963ec11730e7/scratchpad/ADAPTATION_ONE_ENGINE_DESIGN_V1.md` (D, revision 2.1)

This replaces the line of section 3 "NOT in this work order: design steps 8.3 to 8.7"
for the parts that I names. Steps 8.4a, 8.5, 8.6 and 8.7 stay out.

| Item | Content | Results | Text |
|---|---|---|---|
| N8 | The wall geometry law as production code with its census; the requirements record as a view; the table of the switch places; measurement M6b | identical | I part A |
| N9 | Counters of the wall stage (refusal classes, guidance causes, call sites) and one run of the G3 fixture | identical | I part B0 |
| N10 | No progress veto for a needed wall split | BL wall stage changes; no-BL identical | I part B1 |
| N11 | Inside a failing face the law chooses the edge | BL wall stage changes; no-BL identical | I part B2 |

Order with the cut (6.3):
- These items do not need the cut. They name symbols, not line numbers.
- If the cut has not made its first production edit when you read this: do N9, N10, N11
  and N8 first (in this order: N9 and N10 are the ones that help the BL goal most), then
  the cut. Record the cut references (G1, G2, G3, GQ) after them.
- If the cut is in work: finish the phase that is in work (A, B, C or D) with its
  gates, then do N9, N10, N11, N8, then record G1, G2, G3 and GQ again and continue the
  cut.
- A failed item is reverted by R5 and does not stop the items after it, except: N11
  needs N8 part A1 (the law); if N8 A1 is not in, do A1 before N11. N10 does not need N8.


R1 clarification from the user: A heavy process is a solver rank, a build job, a test binary, or a script of ours that uses more than half of a core for more than 60 seconds. Do not start more than 6 of them in total. A browser or another program of the user is not a heavy process of the work. It does not fail a run and is not a reason to repeat a run. Record it in the receipt as an observation only.

Work order: finish 6.1 and 6.2, then N9, N10, N11, N8 (with N8 A1 before N11 if absent), then 6.3, N6, N7. Do not stop or restart a running job.

# Implementation and validation cadence

User order, 2026-11-16 (LATEST): NIGHT_WORK_ORDER.md section6, including 07:50 B7 decision.

## 6. Addition of 2026-11-16 (user decisions; this section wins over the text above)

State when this was written (07:40): N1 FAILED and reverted (`becb4927c9`), N2 NOT STARTED,
N3 PASSED, N4 FAILED at A1 and reverted (`8039fc1b59`), N5 in its one corrected full
repeat, N6 and N7 NOT STARTED. Do not stop or restart the running job. Finish N5 first.

New order of work: finish N5 -> 6.1 (N1 again) -> 6.2 (N2) -> 6.3 (N4 again) -> N6 -> N7.
All rules of section 2 stay. The rule of 6 heavy processes counts your helper scripts too.

### 6.1 N1 again: the user accepts P7 / P8 with a new reference for event 2

Cause of the first failure (your diagnosis, confirmed by the reviewer): the partition
weights of the next event read `prepared_pieces` of the geometry cache
(`CNativeAffineReference.cpp` `StructuralEstimate`, `CNativeImport3D.cpp` 1762-1806,
MAX over the ranks). P7 changes which facets stay in the cache. So event 2 starts with
other weights and gives another valid mesh. This is a property of the proposal, not an
error of your code. The reviewer's statement "identical results" was wrong for event 2.

User decision: P7 / P8 are accepted when event 1 is identical to P6c and event 2 passes
all standing gates and is repeatable. No physical gate, budget or limit changes.

The "P7 reference" for event 2 is the three event-2 files of your first N1 run
(`R/p7p8_r3_20261115/first_result_failure.json`):

- `adapted_mesh_adap_00002.su2`            63690652de1af70f890994fb5054523403ba1dc27200123a660e573827aebc36
- `adapted_mesh_adap_00002.su2.native_ref` 36c45abe03d1dc38e523583cc3103cfb9c83b17983b3679df0c424fab8ff088a
- `restart_flow_adap_00002.dat`            44766f7f95f14d60011fb27ef1bb864c44e9649f782f2b9a0139c6766c93bc94

Do this:

1. Base: the newest accepted commit after N5. Put back the 16 code / config / test paths
   that `becb4927c9` reverted, with their content of before that commit. Keep the N5
   observation code and the N5 fixtures. A conflict in a test file is your own test code
   (R3). A conflict in a production file: both texts stay, P7 / P8 as written in H and
   the N5 observer as it is; write each such place in the report. No other production
   change.
2. Build. Run the unit cases of the item, then the ordered checks (a) and (b) of H with
   the safeguards (exact first collapse, C492, 481, current target, 2D at 1 / 2 / 4
   ranks). They passed before on this code; they run again because the binary is new.
3. Run ONE full two-event workflow. Gates:
   - all standing gates of both events: 5400 s, 19073 MiB (P8: reported job peak equal to
     the sum of the process peaks, lower bounds not above it), physical validator,
     transfer, continued CFD, isolated restart where H asks it, 95 % timer coverage, join
     agreement, P7 check c1 (owned proof faces SUM equal to the surface size);
   - event 1: three files byte-identical to P6c;
   - event 2: three files byte-identical to the P7 reference above. Equal hashes prove
     that two runs give the same files.
4. If all gates pass but the event-2 files differ from the P7 reference: do not revert
   yet. Run the full workflow ONE more time with the same binary and inputs. If the two
   new runs have the same three event-2 files: N1 is PASSED, these files are the P7
   reference from now, and the report has all three hash sets with the note "differs
   from the first N1 run; the build differs by the N5 observation code". If the two new
   runs differ from each other: the result is not repeatable; revert the item, FAILED.
5. If event 1 differs from P6c, or another standing gate fails: R5 as written.
6. After N1 is PASSED: where H, C or this work order says "six files byte-identical to
   P6c", read "event 1 files identical to P6c, event 2 files identical to the P7
   reference". This holds for gate G5 of C and for N5 / N6.
7. The M18, M20 and M21 rows of the accepted run are qualified. Put them in the N5 table
   in place of the unqualified rows. Report the event times and the timer-tree rows
   beside F3b (2683.4 s and 1977.2 s) and beside the first N1 run (2220.3 s, 1451.0 s).

### 6.2 N2 (B7): the user accepts B7 with new references, by the same rule as 6.1

(Changed 2026-11-16, 07:50, second user decision. It replaces the "limited gate" text.)

1. Compute the condition (item 0 of H 2.0c) with the M18 rows of the accepted N1 run.
   Condition false: write the rank values and the total, N2 is "condition false", go on.
2. Condition true: implement B7 as written, build, run the checks of H 2.0c, then one
   full two-event run (run 1).
3. All standing gates of both events must pass (the list of 6.1 item 3). If one fails: R5.
4. Compare the six files of run 1 with the references of 6.1.
   - All six identical: N2 is PASSED. The references do not change.
   - One or more differ: this is expected, no diagnosis pass. B7 changes what stays in
     the cache, and `CNativeEngine3D.cpp` 2957 reads `prepared_pieces` inside event 1
     too, so event 1 can change as well. Run the full workflow ONE more time with the
     same binary and inputs (run 2). All standing gates must pass in run 2.
     - Run 2 has the same six files as run 1: N2 is PASSED. These six files are the
       "B7 reference" from now. It replaces the references of 6.1 in G5 of C and in N6.
     - Run 2 differs from run 1: the result is not repeatable. Revert B7 in one commit,
       FAILED, and go on with 6.3 on the N1 build.
5. Report in each case: the six hashes of each run, the cell and point counts of both
   events, the event times, the RSS sums, the timer-tree rows and the M18 rows, beside
   the accepted N1 run and beside F3b.
6. The memory rule stays as written in H 2.0c: the job peak of each run must be at most
   19073 MiB. A run above it fails a standing gate (R5; the cause is then in the
   proposal, so B7 is reverted).

### 6.3 N4 again, with the corrected gate G1

C has a new section 2.1 (G1 is an identity gate with its own reference). Read C again
completely and copy it with its SHA256 into your evidence folder.

1. Reference build = the build of the newest accepted commit (after 6.1 / 6.2).
2. Record with it, before the first edit: the G1 reference (C 2.1), and G2, G3 and GQ
   again (the production code changed since `4798ea3936`).
3. Then C 2.1 item 5 decides about A1, and the cut goes on as C says. G5 uses the
   newest references: those of 6.1, or the B7 reference when 6.2 made one.
4. The list "unit tests that fail on the accepted base" goes into the morning report.

### 6.4 Morning report (N7)

Keep the file `Handbook/ADAPTATION_NIGHT_REPORT_20261116.md` and update it in place. Add
one row for each item of this section. The first N1 run and the first A1 attempt stay in
the report as history with their numbers.


User order, 2026-11-15 (LATEST): unattended work, section 2 of NIGHT_WORK_ORDER.md

These rules are the latest user order. Where an older text (AGENTS.md, H Part 0) says
"stop and wait for the reviewer", read it as "stop THAT ITEM, record it, and go to the
next item of section 3 that does not depend on it". Add this order to AGENTS.md.

R1. No gate, budget, tolerance or limit changes. 5400 s for each event, 19073 MiB for the
    job, geometry 1e-4, q / J / size / height gates, requested heights and progression.
    One owned heavy job, nice 19, at most 6 heavy processes on the machine. Check the
    host load and the free memory before each long run.
R2. No redesign and no own fix of production behaviour. You implement what H, D and C
    say. A production change that is not written there is not allowed.
R3. Your own test code is yours to make work (H Part 0). Fixtures, scripts and report
    helpers that you wrote: find the cause, correct, run again, until they test what the
    item asks. Limit: 90 minutes for one fixture. Never change production code or a gate
    to make a test pass.
R4. One source check for a whole item, before its first production edit. Write all
    contradictions in one list in the report.
    - A contradiction on a print-only counter or a report row: write it, continue.
    - A contradiction on a result where the written text has one clear reading that the
      source supports: use that reading, write it in the list as "reading used".
    - A contradiction on a result with no clear reading: do not implement that part.
      If the rest of the item is independent of it, do the rest. If not, skip the item.
R5. A failed result gate (outputs not identical where they must be, accept / reject,
    validity, geometry, quality, memory, time):
    - Preserve the evidence. Write the first number that differs.
    - Use ONE diagnosis pass of at most 60 minutes, read-only plus the existing counters,
      to name the cause. If the cause is a transcription or wiring error against the
      written proposal (your code does not do what the text says), correct it one time
      and repeat the failed check one time.
    - If it fails again, or the cause is in the proposal itself: revert the item in one
      commit (keep the evidence and the reverted commit hash in the report), mark it
      FAILED, and go to the next item that does not depend on it.
R6. Time limits, so that one item cannot take the whole night: 90 minutes for one
    fixture (R3); 60 minutes for one diagnosis (R5); 3 hours for item N3. At a limit:
    write the state, go on.
R7. Keep a progress file `R/night_20261115/progress.md` (R as in H). Update it after each
    sub-step: item, sub-step, commit, gate numbers, next action. After each compaction of
    your context, read this work order and the progress file again before you continue.
R8. Commit after each passed sub-step. Push the checkpoints. Never commit a state that
    fails its gates as "accepted"; a preserved failure is labelled as a failure.
R9. Readable code (D 7.1a) for all new code. Written exceptions are listed in C.

User order, 2026-11-15 (LATEST; replaces the "one correction" rule for fixtures):
Your own test code is yours to make work. A test fixture, script or report helper that
you wrote for an item is not production code and not a result gate. When it fails
because of its own setup: find the cause, correct it, run it again, and repeat until it
tests what the item asks. No limit on the count; limit of 90 minutes for one fixture.
Never change production code, a gate or a tolerance to make a test pass. Write each
cause and correction in the report afterwards. Do not stop and do not wait for the
reviewer. Stop only when: (1) a result gate fails in a production run; (2) the source
contradicts a proposal on a result; (3) the corrected fixture shows that the production
code does not do what the item asks; (4) the 90 minutes end.

User order, 2026-11-15 (LATEST; refines the "stop at the first contradiction"
rule below): before any production edit, source-check the WHOLE item (all its
steps, checks and counters, including connected items built together) and report
ALL contradictions in ONE list with source lines/numbers; the reviewer answers
them in one correction. A mismatch in a print-only counter or report row does not
stop the item: report it with call-site counts and continue. Gates on results
(mesh/output identity, accept/reject decisions, validity, geometry, quality,
memory, time) stay strict: stop on their first failure. Still no attempt loops
and no independent redesign.

Reviewer correction supplied 2026-11-15: M6_FIX_HANDOFF.md section1.7 (P6c)
supersedes waiting after P6. Same-fixture baseline scan first; implement the
specified deferred-bound completion/counter/guard records and repeat the ordered
checks once, stopping on first failure. No independent replacement fix.

User order, 2026-11-15 (updated M6_FIX_HANDOFF.md): the reviewer supplies ideas
and design; implement, build, run and report, without independent redesign.
Check proposals against source first. On a contradiction or first failed check,
stop that item, report source lines/numbers and available purpose/call-site counters,
and wait for a corrected proposal. No attempt loops or self-invented follow-up fixes.
P6 is isolated before the cheap group: set aside its open edits, implement section
1.5, then run section 1.6 checks once in order, stopping on the first failure.
No full M6 run until the preceding P6 gates pass. Do not change pinned fixture
values; report changed counts/ranges. Keep F1's build494 count reference until
P6's full two-adaptation milestone passes. Reviewer-owned one-engine design is
read-only; after cheap-group profiling and F3, source-check it and measure M1–M8.
No implementation of its steps8.2–8.7 before user acceptance.

User order, 2026-11-14 (M6 fix handoff): protect shared-engine changes with the
executed gates in `integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57/shared_engine_regression_gates.json`.
For each connected shared-code change, run the affected no-BL first-collapse
work/count gate, exact saved-state BL repeat and native 2D BL/no-BL checks at
1/2/4 ranks. Run the full two-adaptation no-BL workflow at integration milestones.
Keep the stated time/work/resource and physical gates unchanged, and report failed
or unevaluated gates explicitly. Artifact hashes alone prove files unchanged;
they do not establish execution or performance preservation.

User clarification, 2026-11-11 (LATEST CADENCE; supersedes earlier immediate-full-run retries):
Use incremental, stage-based integration. Establish the execution sequence/timing
from source and existing evidence. Work in order: saved CFD/geometry; composed
metric/normalization (finish tangential-profile investigation); coupled surface
preparation; constrained directions/first row; successive growth; outer transition
and strict closure; transfer/continued CFD/second adaptation/isolated restart.
Define each stage's input/output/acceptance before changes. Save credible reusable
stage outputs with complete provenance and enough state to reconstruct the SAME
composed target and geometry; sensor tensors alone are insufficient. Preserve
validity-first provisional outputs with diagnosed quality debt; final gates still
apply before mesh acceptance. Use focused checks and meaningful MPI integration
milestones. Reserve full M6 retries for integration milestones or failures that
cannot be reproduced at a stage boundary. Keep one cycle, one owned heavy job,
budgets, source freezing and the complete final objective unchanged.
Maintain Handbook/ADAPTATION_TIMINGS.md with current per-stage timings/work and
evidence-backed major computational efficiency improvements; keep nested timers,
contention and attribution gaps explicit.

User clarification, 2026-11-11: RANS500k is a NOMINAL metric-complexity target,
not an acceptance gate or exact node count. Explain overshoot and uncertainty;
preserve requested normal heights/progression and all strict acceptance/resource gates.

User clarification, 2026-10-30: the 500k adaptation target was intended for RANS,
not Euler. Use a much smaller Euler target; the current functional-loop attempt
uses 50k (an implementation choice), with 500k reserved for RANS. Keep all geometry,
quality, transfer and requested-height gates strict. Report actual mesh point/cell
counts separately from the metric-complexity target.

User instruction, 2026-10-29: let the complete M6 attempt drive development.
Freeze a coherent implementation, allow generous but bounded certification work,
and attempt the requested CFD → mesh → CFD → mesh workflow. Address construction,
certification or transfer/CFD according to that run's failure; focused checker
changes and tests are diagnostics, not the immediate capability objective.

Permanent user instruction, 2026-10-29 (overnight continuation): if the M6 run
reveals a clear computational bottleneck whose fix will improve efficiency, or a
construction/certification/transfer/CFD error, preserve its evidence, stop only the
verified owned run, implement a connected fix, validate the affected behavior and
rerun ONERA M6. Continue autonomously until a valid accepted Euler
CFD → mesh → CFD → mesh loop succeeds; passing diagnostics are not completion.
Then verify that BL generation still works before proceeding with the RANS grid
and the same target-driven workflow. Keep strict acceptance gates, one owned heavy
workload, resumable evidence and regularly pushed reviewable checkpoints. The user
is unavailable overnight; resolve routine choices within the authorized scope
without waiting for answers. This instruction persists across sessions/compaction.

User instruction, 2026-10-12: prioritize substantial, connected implementation
steps over repeated small checkpoints.

- Group changes that solve the same end-to-end capability before running a broad
  regression. Do not require the full suite after each small edit or checkpoint.
- During implementation, run focused tests when they resolve a concrete failure,
  exercise new risky behavior, or validate a changed interface. Documentation and
  routine low-impact edits do not need a new test campaign.
- Run appropriate broader regression at meaningful integration milestones and
  before declaring the integrated result validated. Never weaken acceptance gates
  or omit necessary correctness checks to satisfy this cadence.
- Reserve MPI campaigns for substantial connected implementation milestones; do
  not repeat them for each small local edit. Use focused serial checks during
  development where they resolve a concrete risk.
- Push reviewable checkpoints regularly. Clearly label incomplete or unvalidated
  work; a checkpoint does not itself require repeating a previously passing suite.
- Updates should state progress toward the actual end-to-end goal and its remaining
  blockers, rather than treating each passing intermediate test as a milestone.

## Capability-first implementation

Permanent user instruction, 2026-10-14; applies across context compaction and
future sessions.

- Organize work around a complete end-to-end capability and its known difficult
  failure, rather than repeatedly extending isolated operators until toy cases pass.
- Before a substantial implementation, state the missing capability, the evidence
  for its underlying limitation, and the acceptance conditions. Choose a connected
  design that addresses that limitation across the affected callers and constraints.
- Use the known difficult target and a small set of meaningfully different fields
  or geometries to assess the complete change. Do not defer the difficult case
  until a succession of easier checkpoints has passed.
- Simple tests establish primitive correctness or resolve specific risks; they do
  not establish robust mesh generation and must not be reported as capability
  milestones. Keep private intermediate progress distinct from an admissible final
  mesh and a successful adaptation/solution-transfer workflow.
- The next meaningful curved-construction milestone is a complete accepted mesh
  under unchanged constraints. If the current recovery cannot deliver it, reconsider
  the construction strategy before adding another local fallback.
- When a strategy stalls on the difficult target, preserve the diagnosis and
  reconsider the construction or coordination it lacks. Do not keep adding local
  variants or raising limits without evidence that they address the blockage.
- Deliver substantial connected improvements, then validate at the cadence above.
  Preserve the full agreed goal and all acceptance gates; neither a prototype nor
  a passing fixture justifies narrowing the goal or declaring it complete.

## Reuse and resumable validation

Permanent user instruction, 2026-10-13; applies across context compaction and
future sessions.

- Reuse existing implementation and validation facilities before adding new ones.
  Keep implementation and checks tied to the full agreed goal and its exclusions.
- Reuse passing evidence when its scope covers the requirement and the relevant
  source, build/binary, configuration, inputs and validation helpers remain valid.
  Verify the needed provenance; source hashes alone do not prove unchanged runtime
  behavior. Rerun checks affected by changes, failures or unresolved concerns.
- Make long validation campaigns resumable: save complete independently validated
  cases or stages with their provenance as they finish. A timeout must not discard
  completed evidence or force successful work to repeat. Partial results cannot
  establish whole-campaign success.
- Resume a saved result only after verifying its relevant hashes and acceptance
  conditions. Preserve failures and diagnostics, keep resource/work limits explicit,
  and require every planned case or stage before declaring completion.
- Before restarting a job, verify its existing process/session/job handle. Elapsed
  time, an observation timeout or stale logs alone do not establish termination.
- Keep compiled/executing/pinned C++ sources unchanged throughout a build or validation
  campaign. Verify the owned handle is terminal before editing those sources. Git
  snapshots and documentation updates are allowed without changing the pinned code.

## Adaptation architecture

User instruction, 2026-10-14: Euler-to-BL and BL-to-Euler describe validation
scenarios, never production transition modes. Mesh construction and removal must
follow the target metric, geometry authority and currently requested wall-height
constraints through general remeshing operators. Do not dispatch reconstruction
from Euler/RANS labels, an inferred input-mesh class, case names, saved coordinates
or fixture identities. Solver compatibility and solution-transfer checks remain
separate from mesh reconstruction. Wall-aware operators are allowed to enforce
actual geometric constraints; limited fixture success does not establish general
robustness. Qualify wider fields/geometries rather than adding scenario switches.

## Handbook maintenance

User instruction, 2026-10-14: keep HANDOFF_Codex.md current and concise. Put detailed
current topics and accumulated history into smaller linked handbooks when the entry
point grows. Preserve findings, failures and evidence; historical job/status notes
must not be mistaken for current state. Update the current state in place rather
than prepending repeated copies of prior updates.

## Resource discipline

User instruction, 2026-10-17: account for machine contention and RAM usage.

- Check actual host load and available RAM before sustained computation. Run one
  owned heavy job at a time; use low priority,2–4 build workers when resources permit
  (user update2026-10-29) and single-threaded
  libraries. Keep local MPI correctness checks bounded to available ranks/resources.
- Check live job handles before starting another job. Observe process memory during
  longer runs, preserve resource receipts, and leave unrelated processes untouched.
- Use the repository build's compile-tmp on the4TB disk while the root/tmp filesystem
  is nearly full. Do not infer whole-job peak memory from one process's RSS.

## Optional local second opinion

User authorization, 2026-10-18: consult the local Claude Code installation when
useful, using Opus 5.5 at medium or high effort. Keep source-checking and validation
with the implementation; a model review does not establish correctness.

## Staged adaptation and boundary mobility

User reaffirmation,2026-10-26: implement the already agreed validity-first construction
followed by metric/BL-aware quality improvement. q/J/size and requested first-height gates apply to the
final accepted mesh; intermediate meshes must remain valid. Fixed physical boundary
node coordinates are not required. Allow redistribution on the prescribed geometry
while retaining genuine features, body separation and requested BL constraints.
Complete the connected distributed workflow rather than accumulating strict local
construction fallbacks; preserve all final acceptance/transfer/restart obligations.


User clarification,2026-10-26: construction must omit q/J measurements, shape
ranking and quality-progress vetoes that are unnecessary for valid elements.
Exact positive, nondegenerate cells, topology, geometry and body separation remain
mandatory; J>=.001 is a quality threshold, not geometric validity. Measure and
improve shape in the subsequent stage and enforce all final acceptance gates.
Delegate bounded routine tasks to more token-efficient models; retain integrated
design, source review and acceptance assessment with the primary agent.
