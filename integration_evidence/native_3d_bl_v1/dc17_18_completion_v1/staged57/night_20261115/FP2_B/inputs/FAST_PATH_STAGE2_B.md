# Fast path stage 2, item FP2-B: the core repairs the volume beside its surface splits, and step D does not cut beside a locked face

Author: reviewer, 2026-11-21 (checked against the source and your evidence). For the main agent. The user sends it; it
is a user order. Copy this file with its SHA256 into `R/night_20261115/FP2_B/inputs/`.
One whole-item source check before the first production edit (R4). Questions: R13.

Short names of files: as in `FAST_PATH_STAGE2_B0.md` ("B0"): `Core.hpp`, `Surf.hpp`, `Mesh`, `Changes`, `Pass`,
`Shift`, `Measures`, `Surface`, `Geo`, `Lab`, `Run`. "The text of FP2-A" = `FAST_PATH_STAGE2_A.md` with its corrections
1 to 4. Line numbers are those of commit `852e566d17` for production files and of your FP2-B0 commit for test files.

## 1. Why

Your FP2-B0 measurement was complete and exact. What it showed:
- The surface splits make the bad cells: in H1 the surface steps add 24,078 bad cells, all with a new surface point.
  The quality work has no operation for most of them. At the hand-over: 2,538 (surface off) / 11,099 (surface on).
- Your two test operations bring this to 3,741 (X6). The V-splits open the cells for the quality work of the core:
  the V-splits alone raise the count, and the quality work after them gives the whole gain. V-collapse adds 14.5 %
  (classes (b) and (d)). Each run ended with a round without a V operation, never at the limit of 10.
- With 6 ranks step D made 18 edges with a metric length below 1e-2 (the smallest: 2.5e-4, 1.0e-5 in space). Each split
  was at the midpoint, so the placement is not the cause. The cause is a chain (H6, rank 4, pass 1, sweeps 4 to 13):
  from sweep 2 the face beside the edge 18363-18371 has that edge as its longest edge; the edge is never split (the
  reviewer infers: it meets the border of two ranks); the face is then cut again and again through its SHORTER
  edges, because the face on the other side asks for that edge. Each cut halves the distance to one end. H1 (no rank
  border) has no such edge.
This item puts the two operations in the core as you tested them, closes the chain by a rule, tests in the lab what
the engine gets (a hand-over gate), and then repeats run S1.

## 2. Limits of the item

- All new behaviour is under `options.surface_splits` (option `ADAP_NATIVE_LOCAL_SURFACE= YES`). With the option NO
  each result stays identical (section 6). Write new functions only. Do not change `Split`, `CollapsePlan`,
  `CanCollapse`, `Prepare`, `Commit`, `QualitySweep`, `QualityWork`, `LengthWork` or `SweepCounters`.
- The core runs only without wall heights (`applicable`, `Run:699`), so the collapse of a wall-near free point is
  permitted here. Write this reason as a comment at the new collapse.
- No change of the engine, of `Limit()`, of the tolerance, of the target, of step L, of a limit of the core.
- R9 (readable code): the names below are proposals; small functions; comments say why.

## 3. Part A1: the volume repair rounds (production)

Move the logic of your run X3 into the core. Items 1 to 3 below are written from your test code of FP2-B0
(`native_handover_volume.inc`, lines 34-230); gate E asks for the same numbers. If you find a decision of items 1 to 3
that differs from that code, use the code, write it as "reading used" (R4) and as a question (R13). This rule does
not cover the place and condition of the call (item 3, first sentence), the prints (item 4), or section 4.
1. `Mesh::SplitInside(seed, ...)` (the V-split), for one bad cell: try its six edges from the longest to the shortest
   in space (`std::hypot` of the coordinate differences; equal lengths: the smaller sorted pair of point ids first).
   Skip an edge when its ring (`EdgeRing(edge, seed)`) is not `Closed()`, or has a cell that is `Blocked` or carries
   the stamp of this round. Else sort the ring cells by `Key` and APPEND the trial row
   `{{point_ids.Peek(), midpoint in space}, Midpoint(tensor a, tensor b)}` at the end of `points_`. Do not reuse a
   slot of `free_points_`; this differs from `Split` (`Changes:271-274`) because your test code appended and gate E is
   exact. The new cells: for each ring cell in this order, the cell with the first end (id order) replaced by the new
   point, then the cell with the second end replaced. With the trial row in place, the checks are:
   `Fits(old, new, cell_limit)`; each edge of the new cells that is not an edge of the ring cells has
   `Length <= LongEdge`; the smallest `Margin` of the new cells is strictly above `Worst` of the ring cells. Then
   `Prepare(ring, new)` must accept and `ExactlyPositive(plan)` must hold. Then `Commit(plan, stamp)` and
   `point_ids.Take()`. On any refusal, remove the trial row and try the next edge. The first accepted edge ends the
   cell. No length condition and no progress test.
2. `Mesh::CollapseNear(seed, ...)` (the V-collapse), for one bad cell: take its points in id order. Skip a point p
   that is not `Free()` or not `wall_near`, and skip p when a cell of `Star(p)` is `Blocked` or carries the round
   stamp. For each neighbour k of p (each other point of a cell of `Star(p)`: surface, border or free), build the
   new cells as `CollapsePlan` does (`Changes:181-191`: the cells of `Star(p)`, in that order, without the cells that
   hold k, with p replaced by k in place). Keep k when the three checks of item 1 hold against `Star(p)`. Try the kept
   k with the larger worst new margin first (equal margins: the smaller id of k first). The first k for which
   `Prepare` accepts and `ExactlyPositive` holds is committed with `Commit(plan, stamp)`; `Commit` removes p
   (`Mesh:315-320`). The first commit ends the cell. If no k of p is committed, go to the next point p. k does not
   move. Do not call `CollapsePlan` or `CanCollapse`: they refuse a wall-near p (`Changes:173`, `:180`).
3. `Mesh::RepairRounds(point_ids, cell_limit, stats)`, called in `RunPass` directly after `QualityWork` and before
   `check_start` (inside the time of `seconds_quality`), when `options.surface_splits && surface` (the condition of
   `SurfaceWork`, `Pass:430`). A round is: `++sweep_` (the stamp of every change of the two V sweeps of the round);
   `MarkWallNear()`; the V-collapse sweep; the V-split sweep; `QualityWork(point_ids, cell_limit, stats)`.
   Each V sweep builds its own list from the mesh as it is at the start of that sweep: the live cells with
   `Margin < BadMargin` that have a point with `surface` or `border`, without the cells that carry the round stamp,
   in ascending `Margin`, then `Key` (the order of `QualitySweep`, `Pass:304-306`). A cell of the list that is dead or
   carries the round stamp when its turn comes is skipped. `QualityWork` runs in every round, also in the round that
   accepts no V operation. Stop after the first round that accepts no V operation, or after 10 rounds (a named
   constant with the reason: your runs needed at most 9; the limit only bounds a fault).
   Comment for the acceptance rule: each accepted operation strictly raises the worst margin of the cells that it
   replaces, so a round cannot cycle.
4. Print (R11; computed only when `stats.report`): fields of `SurfaceCounters`, printed in the row
   `NATIVE3D_LOCAL_SURFACE` by `PrintSurfaceFields` (not in `NATIVE3D_LOCAL_CORE`, not in `SweepCounters`):
   `v_rounds=` (rounds run, the last one included), `v_limit=` (1 when round 10 accepted a V operation),
   `v_collapses=`, `v_splits=`, `v_changes=` (one entry for each round, `c<collapses>s<splits>q<quality sweeps>`,
   comma separated, like `round_changes`), `bad_before_v=` (the bad cells after the first `QualityWork`),
   `seconds_v=`. No decision reads them.

Gate E (equivalence with your measurement; build with A1 only, WITHOUT part A2): new hidden cases with the inputs of
B0 that run the PRODUCTION `RunPass`, in the flow of LAB-S (1 rank) and of the rank check (6 ranks). The B0 cases and
their test operations stay as evidence. Exact numbers:
- one rank = X3: 3,756 bad cells at the end, 561,067 cells, 84,102 faces; `bad_before_v=11296`, `v_collapses=172`,
  `v_splits=1175`, `v_rounds=7`, `v_changes` equal to your `FP2_B0 volume_round` rows of X3;
- six ranks = X6: 3,741 bad cells (the sum of `bad_out` of pass 2), 556,192 cells after the higher shift, the sums of
  `faces_out` 84,124 / 84,184; the `v_changes` of each rank and pass equal to X6; the sums of collapses and splits
  209 / 1,237; `bad_before_v` summed over the ranks 11,644 (pass 1) / 3,843 (pass 2).
If a number differs: the first differing entry of `v_changes` (by rank, pass, round) names the place; this is a
transcription error (R5: correct one time).
Fixtures (small, serial): a V-split that is accepted keeps each physical face and binding; a V-split with no rise of
the worst margin is refused and leaves the point rows as before; a V-collapse onto a surface point keeps that point
bit-equal; a pass with no accepted V operation gives `v_rounds=1` and `v_changes=c0s0q<n>`; with `surface_splits` off,
`RunPass` gives `v_rounds=0` (report on) and the FP1 fixtures pass unchanged.

## 4. Part A2: step D does not cut an edge beside a locked face (production)

Definitions for a sweep of step D.
An edge e is LOCKED when (1) it is in `refused` or `border` of this call of `SurfaceDistance` (`Surface:409`; read as
they are at the start of the face loop of the sweep; the sets of step L do not count), or (2) its ring meets a
rank-border face: `EdgeRing(e, c).border` for a live cell c that holds e (the first test of `SplitSurface`,
`Surface:226-227`).
The other face at an edge e of a physical face f: the faces of `SurfaceParents(e, EdgeRing(e, f.cell))`
(`Surface:191-199`). When this gives exactly two faces, the other face is the one that is not f. Otherwise there is no
other face and the rule does not apply.
The longest edge of a face: in space; equal lengths: the smaller sorted pair of ids (correction 3; `Surface:473-477`).

Rule A (item 2 of step D; only in the branch where E is the longest edge of the face, not in the branch of a feature
edge with a chord above `Limit()`). After the tests refused / border / short (`Surface:483-488`) and before "mark E":
if the other face at E exists, and its longest edge is not E and is LOCKED, mark nothing for this face in this sweep.
Give the face the reason `neighbor_locked` (a new value of `SurfaceRemaining`, `Core.hpp:102`) and do not add it to
`chosen`. Else mark E as before.
Rule B (item 1 of step D). A feature edge f with a chord above `Limit()` that passes the tests of item 1 is not marked
when one of its physical faces (those of `SurfaceParents(f, EdgeRing(f, c))`; on a rank border there may be one) has
a longest edge that is not f and is LOCKED. In item 2, a face whose selected
feature edge passes refused / border / short but is not in `marks` gets the reason `neighbor_locked` and is not added
to `chosen` (else `marks.at`, `Surface:530`, throws).
Count `left_neighbor_locked=` in the row: the faces above `Limit()` at the end of step D with this reason
(`Surface:534-547`). Add the new value to every `switch` over `SurfaceRemaining` (`Surface:540`, `Lab:261-266`) and, in
test code, to the census of `ClassifyLabSurface` and `reason_accounting` (`Lab:306-307`), so that the build has no new
`-Wswitch` warning. In the FP2-A cylinder fixture the sum of the reasons
(`left_distance == left_refused + left_limit + left_short`) gets `+ left_neighbor_locked` (test code).
Step L does not change (its chain ends when the edges are short in the metric).
Comment at the rule: a face whose own longest edge cannot be split is cut through a shorter edge only by its
neighbour; each such cut leaves it with the same longest edge and halves the cut edge toward one end (the chain of
FP2-B0, 18 edges below 1e-2). The asking face stays for pass 2 (after the shift a border edge is inside a rank) or for
the engine.

Fixtures (serial):
(a) A box with a rank-border face (`Mesh(owned, true)`): two physical faces f and g above the limit share E, the
    longest edge of f and not of g; the ring of the longest edge of g meets the `Border` face. Step D marks nothing
    for f, `left_neighbor_locked=1`, and E is not tried. The same box without the `Border` face: f marks E and E is
    split as before.
(b) The refused part: with `refused` = {the longest edge of g}, the face-selection helper of item 2 (a small function
    of its own, R9) returns `neighbor_locked` for f, and with the set empty it returns E.
(c) Rule B: a feature edge beside a LOCKED longest edge is not marked, and the pass ends without an exception.

## 5. Part A3: the hand-over gate in the lab (after A1 and A2; BEFORE a full run)

Runs N1 (1 rank, the input of H1, the flow of LAB-S, `Lab:613-696`) and N6 (6 ranks, the input and unit weights of
H6, the flow of the rank check, `Lab:448-515` and `698-787`), with the PRODUCTION `Mesh::RunPass` and the surface
option (no copy of `RunPass`, no recording wrapper). Measurements are read-only, through the friend, after `RunPass`
returns and before `WriteBack`:
- B0 3.1, the totals by class at the end of each pass (no c3; in its place the field `bad_before_v`);
- B0 3.3, the edges (the 20 shortest and the counts below 1e-6 / 1e-4 / 1e-2 / 0.1);
- B0 3.4, the histograms and the 20 largest at the end of the last pass;
- B0 3.6 after the last shift (N1: after the pass).
Print the row `NATIVE3D_LOCAL_SURFACE` after `RunPass` returns (as LAB-S, `Lab:658-662`), not through `surface_sink`
(that sink fires before the volume work, so the `v_` fields would be 0).
N1: then steps 2 to 4 of section 8 of FP2-A and the strict proof, exactly as LAB-S.
N6: then `BuildSurfaceState`, the check of 8.4 (`CheckRankOwnedGeometry`) and the `Snapshot()` agreement as K3. Then
the strict proof once, on rank 0, on the built state. Its reason map is the union over the ranks of the faces that
`surface_remaining_sink` reported in PASS 2. Each physical face is on one rank in pass 2, and step D of pass 2
samples every face of its rank. Gather it in test code: each rank packs its rows (three ids and the reason), then for
each rank r one `CPassiveComm::BcastRounds(rows, r, world.comm)` (as `Lab:557`); rank 0 keeps the union. A key that
comes from two ranks is a test failure. On rank 0, compute the proof and the census into flags without `REQUIRE`: use
a copy of `ProveLabSurface` that returns them (a `REQUIRE` that fails on rank 0 alone leaves the other ranks waiting).
Then all ranks evaluate `REQUIRE_FALSE(world.maximum(!qualified))` and print the census line of rank 0.
In both runs the census counts `neighbor_locked` as a reason. Wrapper limit: 45 minutes for each run.

"Hand-over gate passed" = all of:
1. each cell exactly positive; the check of 8.4 passes; the proof condition of LAB-S (correction 3 section 3 item 1:
   the proof accepts each entity, OR each face with `Classify` above tol is a face that step D left with a reason)
   holds; with 6 ranks the bytes of `Snapshot()` are equal on all ranks;
2. no edge with a core metric length below 1e-2 at the end of the last pass (H1 and H6off have none; H6 had 18);
3. the bad cells at the end: in N1 `bad_out` of the pass; in N6 the sum over the ranks of `bad_out` of pass 2 (the end
   of `RunPass`, before the higher shift; X6 counted 3,741 so). Not above 4,000 in either run (your X3 and X6: 3,756
   and 3,741; rule A changes a few splits; this is an alarm for a lost repair, not a quality target);
4. the alarms of the rank check as corrected: `star_max` not above 1,000; `sweeps_distance` below 40; the sum of
   `faces_out` not above 126,153 in each pass; no pass above 600 s on a rank; `v_limit=0` on each rank and pass.
Report beside H6off / H6 / X6 (and H0 / H1 / X3): bad cells by class, cells of each rank, the edge counts, the
smallest edge with its 3.3 row, stars, `left_neighbor_locked` and the other `left_` counts, the census line of the proof
(`classify_left_refused / _short / _border / _limit / _neighbor_locked / _unexplained`), the `v_` fields and seconds
by rank and pass.
If the gate fails: one diagnosis pass (R5, at most 60 minutes, read-only). No correction and no revert: the commits of
A1 and A2 stay, labelled "default path not yet proven". Commit the evidence ("FP2-B lab: stopped before the full
run"), do not start a full run, stop and wait for the user.

Build `SU2_CFD` and `UnitTests/test_driver` without a new warning; write both hashes. All fixtures of FP1; fixtures
1 to 4, 6 and 7 of section 10 of FP2-A (not LAB-S and not the rank check: the hand-over gate replaces them; their
numbers change with A1); G1 identical (375 / 114 / 228). Commit and push: "FP2-B part A (lab passed; default path not
yet proven)".

## 6. Part B: run S1 first, then the default proof

Reason for the order: run S1 is the test that decides; the default proof needs about 3 hours and is necessary only for
code that stays.
1. Run S1 on the binary of part A: the config `R/night_20261115/FP2_A/run.cfg`, SHA256
   `07b91a94996bf3cb48352041d8e1643ffe17a61bed07432085cfcf87b1725382` (both options YES). Copy it into `FP2_B/`, check
   the SHA256, and derive the config of the isolated restart from it as in FP2-A 11.3. All standing gates of both
   events (as FP2-A 11.3 item 1).
2. If S1 fails a standing gate: ONE diagnosis pass (R5; for event 1 give the rows of item 4 below beside S1 of
   build627 and CORE of build627, and the q / J / geometry counts over time). No correction. Mark "FP2-B ON: FAILED".
   Then make the tree safe: revert in ONE commit ("FP2-B reverted; production sources = build627") every change of
   this item outside the evidence folder, including the test code that calls the new functions (else `test_driver`
   does not build). Verify three things: the SHA256 of each path of `R/night_20261115/FP2_A/build627/source_pins.json`
   that is not under `UnitTests/` (0 differences); `git diff --stat 852e566d17 -- Common SU2_CFD` is empty (the pins do
   not list a new file); `UnitTests/test_driver` builds. Keep the evidence and the reverted hashes in the report. Stop
   and wait for the user. No S2.
3. If S1 passes all standing gates: the default proof on the same binary: items 1 to 4 of FP2-A 11.2 (fixtures, G1,
   G2, GQ, G3, G4, the safeguards, G5 with both options NO, run CORE) with their references. If a gate of the default
   proof fails: one diagnosis pass, no correction (S1 must have run on the binary of the proof), mark "FP2-B: FAILED",
   revert as in item 2, stop and wait.
   Then run S2. Then the rules of FP2-A 11.3 items 2 to 4 hold with these names: the two runs S1 and S2 give the same
   six files -> write them in `R/night_20261115/FP2_B/on_surface_reference.json`; time rule: event 1 of S1 and of S2
   not above 1.05 times event 1 of the run CORE of THIS binary; a miss of the time rule alone is "OPEN for the user",
   not a failure.
   If S2 fails a standing gate, or S1 and S2 do not give the same six files: ONE diagnosis pass (R5), mark "FP2-B ON:
   FAILED", keep all code (the default is proven; the option stays NO by default), stop and wait for the user. No
   repeat by your own decision.
4. Report rows: FP2-A 11.4 (run CORE beside S1 and S2). For event 1 of S1, each stage, rank and pass: the row
   `NATIVE3D_LOCAL_SURFACE` with the new fields, the sum of `faces_out`, and `short_out` and `wall_near_points` of the
   row `NATIVE3D_LOCAL_CORE`. Beside S1 of build627: the engine seconds of construction and of improvement, q / J at
   `quality_entry`, `geometry_bad` at `length_collapse_exit`, the smallest metric edge of the first `NATIVE3D_LENGTH`
   row of each rank, the `length_collapses` of the engine, the count of ownership migrations.

## 7. Report and close

`R/night_20261115/FP2_B/reports/FP2_B_<build>.md`: the source check (R4, one list), gate E, the fixtures, the hand-over
gate with its table, part B with the rows of section 6 item 4, each correction of test code (R3), each question (R13).
Update the progress file, `HANDOFF_Codex.md`, `HowToProceed.md` and (if S1 and S2 passed)
`Handbook/ADAPTATION_TIMINGS.md`; write there that `seconds_quality` of FP2-B includes the V rounds and that of FP2-A
does not. Commit, push, verify that no owned job runs, stop and wait for the user.

## 8. Not in this item

A surface collapse, swap or move. A change of the engine (its progress rule too). Edge removal for rings above 7. A
change of `MovePoint`. A change of `Limit()`, of the tolerance, of the target, or a size from the curvature of the wall
(parked by the user). A new timer operation or phase. A change of a gate.
