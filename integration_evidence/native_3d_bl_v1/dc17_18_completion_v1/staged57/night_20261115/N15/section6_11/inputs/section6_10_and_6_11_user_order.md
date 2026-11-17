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
