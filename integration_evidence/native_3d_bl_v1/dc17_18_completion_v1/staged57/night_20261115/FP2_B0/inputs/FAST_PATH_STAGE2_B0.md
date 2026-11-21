# Fast path stage 2, item FP2-B step 0: measure what the core hands to the engine after the surface splits

Author: reviewer, 2026-11-21 (checked against the source and your evidence). For the main agent. The user sends it; it
is a user order. Copy this file with its SHA256 into `R/night_20261115/FP2_B0/inputs/`.
This item is a MEASUREMENT. Test code only (R3). No production change (R2). In the report: the SHA256 check of every
path of `R/night_20261115/FP2_A/build627/source_pins.json` that is not under `UnitTests/`, with 0 differences;
`git diff --stat 852e566d17 -- Common SU2_CFD` is empty; the `SU2_CFD` binary hash is unchanged. Build only
`UnitTests/test_driver`. No gate G1 to G5 is necessary.
The option `ADAP_NATIVE_LOCAL_SURFACE` stays in the code with its default NO; "FP2-A ON: FAILED" stays as it is.

Short names of files in this text: `Core.hpp` = `Common/include/adaptation/CNativeLocalCore3D.hpp`; `Surf.hpp` =
`Common/include/adaptation/CNativeLocalSurface3D.hpp`; `Mesh`, `Changes`, `Pass`, `Shift`, `Measures`, `Surface` =
`Common/src/adaptation/native_local/CNativeLocalCore3D<name>.cpp`; `Geo` = `CNativeLocalSurfaceGeometry3D.cpp`; `Lab` =
your lab test file (the file of `[.][NativeLocalCoreSurfaceLab]` and `[.][NativeLocalCoreSurfaceRankCheck]`); `Run` =
`Common/src/adaptation/native_engine3d/NativeRemeshRun.inc`. Line numbers are those of commit `852e566d17`.

## 1. Why

Your run S1 and your diagnosis were correct. The reviewer's reading of your logs (run S1 beside run CORE, event 1):
- The engine accepts the surface of the core: its construction is 259 s shorter (269 s against 528 s), and it starts
  with fewer geometry failures (`geometry_bad` at `length_collapse_exit`: 60 cells against 2,972 over the six ranks; on
  rank 0, 26 against 461).
- The volume beside the new surface points is poor at the hand-over: the `bad_out` of the core after pass 2 sums to
  13,339 against 2,629, and 13,220 of them have a surface or border point. On ranks 0, 1, 4 and 5 each sweep row of the
  length work of pass 1 is equal in S1 and CORE, so there the extra bad cells come from the surface splits alone. In the
  last quality sweep 93 to 100 % of them are skipped by the failure memory: the quality work has no operation for them.
- The engine then needs 1,410 s of improvement (829 s in CORE) and ends in a cycle on one marginal cell.
- The first `NATIVE3D_LENGTH` row of the engine shows metric edges of 5.5e-10 and 1.8e-5. It is not known if they exist
  at the end of the core (that row comes after the first collapse pass of the engine).
The labs of FP2-A tested the core's own results, not what the engine gets. That was a gap of the reviewer's text.
The next production text needs numbers that the logs do not have. This item gets them, and it tries two pure volume
operations in test code before any production design.

## 2. Setup

Test code in `Lab`, with the friend `SU2NativeLocal3D::SurfaceFixtureAccess` (`Core.hpp:250`) for the private state.
Define the friend once, in a new test header (for example
`UnitTests/Common/adaptation/native_local_fixture_access.hpp`).
Move the existing `PrepareSplit` (`CNativeLocalCore3DSurface_tests.cpp:17-46`) into it unchanged. Include the header in
both files. Then run the cases of `CNativeLocalCore3DSurface_tests.cpp` once (serial) and report that they pass.

Inputs: 1-rank runs use `R/night_20261115/FP2_A/scoped/build627/lab/LAB1_V2/` (its `run.cfg`, `m6.su2`,
`m6.su2.native_ref`, restart). 6-rank runs use `R/night_20261115/FP2_A/rank_check/build627/K3/`. Sensor for both:
`R/night_20261115/FP0/cases/B/sensor.bin`. Add new hidden test cases. Do not change the cases
`[NativeLocalCoreSurfaceLab]` and `[NativeLocalCoreSurfaceRankCheck]`; they are FP2-A evidence. Move shared helpers
unchanged.

Sequence. Runs with 6 ranks: base them on the rank check (`Lab:698-787`, passes as in `RunSurfaceWorkerPasses`,
`Lab:448-515`): meeting, pass 1, exchange, shift (lower), meeting, pass 2, exchange, shift (higher). Runs with 1 rank:
base them on LAB-S (`Lab:613-696`): ONE pass on `Mesh(cells, false, options)`, no meeting, exchange or shift, as
production does on one rank (`Run:869`). In sections 3 to 5, "the last pass" is pass 2 with 6 ranks and pass 1 with
1 rank. On 1 rank class (c) of 3.1 is empty.
The state build of section 8 of FP2-A and the strict proof are NOT necessary in these runs (leave them out; write that
you did).

The pass. In test code, reproduce `Mesh::RunPass` (`Pass:420-443`) step by step through the friend, in the same order:
set `options_`, count `bad_in`, `SurfaceWork` (surface on only), checkpoint c1, `LengthWork`, c2, `QualityWork`, c3, the
rounds of section 4 (X runs only), c4 (X runs only), `Check`, `MarkWallNear`, `ReportResult`. Use the same sinks as the
rank check. For a run with surface off, c1 is the start of the pass.
All measurements of section 3 are read-only. They may call only `const` functions (for example `Margin`, `Length`,
`Edges`, and through the friend `EdgeRing`, `Star`, `Blocked`, `Fits`, `Worst`, `TrialPositive`, `Positive`,
`ExactlyPositive`, `RemoveEdgePlan`, `SwapFacePlan`, `CollapsePlan`, `Prepare`), and the public
`SampleMovedTensorFloat`. Never call `MovePoint`, `RemoveEdge`, `SwapFace`, `Collapse`, `Split` or `ReplaceCells` in a
measurement (they commit when they succeed). The one write that is allowed: for the move test of 3.2, set the point
row to the trial value and restore it bit-equal before the next call, as `MovePoint` does (`Changes:487-516`).

The RECORDING wrapper: a class derived from `SurfaceGeometry` (`Surf.hpp:36-47`; `StateSurfaceGeometry` is `final`). It
forwards each call to the `StateSurfaceGeometry` and is passed to the surface steps in place of it. Keep using the
concrete object for `ResidentBytes`, `SetCallerBytes`, `PeakBytes` and `Exchange`.
A split is ACCEPTED when `Accept` is called (`Surface:326`); `Place` is only a proposal. Keep the arguments of the last
`Place` and pair them with the next `Accept`. For each accepted split record:
- the ends a, b (ids, coordinates) and the new point m (`placement.point.node`);
- feature if `placement.parent` has a value, else sheet (`Surface:321`);
- the step: L during `SurfaceLength`, D during `SurfaceDistance`. Set a flag between the two calls: call them
  separately through the friend in your copy of `SurfaceWork` (`Surface:561-589`; keep its order: `SurfaceLength`,
  `SurfaceDistance`, `MarkUnresolved`, the report part);
- for D: "by a feature chord" if `Feature(edge)` is true and the value that `Chord` returned for that edge is above
  `Limit()`; else "by a face" (`Surface:443-453`, `:466-471`, `:490`).
For 3.5, take the coordinates of the third vertex of each of the two faces passed to `Place` from
`StateSurfaceGeometry::Find(id)` (`Geo:477-478`, `:627`). It holds the original and the created surface points.
At `Accept` the mesh already holds m: `free_points_` is empty during the surface steps (check it), so m is the last row
of `points_`. Its new cells are `Star` of that row.

Partition of the 6-rank runs: the unit weights of the rank check (`Lab:721-723`), as K3. The production weights need
the live event-1 metric, which the lab does not have. Do not build them. Section 3.6 gives the regions of the ranks.
Note: the lab partition and metric differ from S1, so the 5.5e-10 edge of S1 need not come back.

One owned job at a time (R1); a run with 6 ranks uses all 6 heavy processes. Wrapper limit 30 minutes for each run.

Runs:

| Run | Ranks | Surface splits | Extra |
|---|---|---|---|
| H1 | 1 | on | measurements 3.1 to 3.6 |
| H0 | 1 | off | 3.1, 3.3 (edges only), 3.4 |
| H6 | 6 | on | 3.1 to 3.6 |
| H6off | 6 | off | 3.1, 3.3 (edges only), 3.4, 3.6 |
| X1, X2, X3 | 1 | on | section 4 |
| X6 | 6 | on | section 4, the best of X1 to X3 (the smallest count of bad cells at c4; write your choice) |

Sanity (a report, not a gate):
- H1 after its one pass gives the cells and the `faces_out` of LAB-S of build627 (554,558 / 84,102).
- H6 gives K3 of build627: 549,519 cells after pass 2 and the higher shift (the sum of `output_cells`); the sums over
  the ranks of `faces_out`: 84,124 in pass 1, 84,184 in pass 2.
If not: write the first number that differs. Your copy of `RunPass` is then the first suspect (R3).

## 3. Measurements

Checkpoints in each pass: (c1) after the surface steps, (c2) after the length work, (c3) after the quality work;
(c4) X runs only: after the rounds of section 4.
A bad cell is a cell with `Margin < BadMargin` (`Measures:181-185`, `Core.hpp:40`), as the core counts `bad_out`.
The input point base = the `MAX` over the ranks of `input.point_base` (as `Lab:451`). The origin of a point: input
(id <= the base); surface split (`surface` and id > the base); other new point (not `surface` and id > the base).
The group of a point, the first that applies: surface (`surface`), border (`border`, not `surface`), free (`Free()`).

### 3.1 Bad cells by class
For each rank, pass and checkpoint, a table of counts of bad cells by:
- class: (a) has a point with `surface = true` and id > the input point base (this is exactly a point of a surface
  split of any pass and any rank); (b) no such point, has a `surface` point; (c) no `surface` point, has a `border`
  point; (d) all 4 points `Free()`. The count of (d) must equal `bad_out_free` at the end of the pass (c3; c4 for X
  runs). Write it if it does not;
- count of `surface` points of the cell (0 to 4), and has a physical face or not;
- made by a surface split of THIS pass and not changed since: `stamp` in (s0, s1], where s0 and s1 are `sweep_` just
  before and just after `SurfaceWork`. A later change of the cell, or a move of one of its points (`Changes:527`), gives
  it a later stamp. At c1 this is exactly "made by a surface split";
- shape: shortest edge / longest edge (in space) in the bins below 0.1, 0.1 to 0.3, above 0.3; and for a cell with a
  physical face: height of the opposite vertex above that face / longest edge of that face (for a cell with more than
  one physical face, the smallest value over them), in the bins below 0.3, 0.3 to 3, above 3.
At c3 also, for each class:
- how many satisfy the memory test of `QualitySweep`: `failed_at != 0` and `failed_at >= changed_at` for each of its
  4 points (`Pass:313-315`);
- how many are `Blocked` (`Mesh:332-336`; a blocked cell is not a quality candidate, `Pass:300`);
- the histogram of `Margin` of the bad cells (bins below 0.01, 0.01 to 0.1, 0.1 to 0.5, 0.5 to 1, 1 to `BadMargin`).

### 3.2 Why a bad cell stays bad
For each bad cell at c3 of the last pass (all of them), test each option of `QualitySweep` in its order
(`Pass:320-366`). Record the FIRST check that refuses it:
- Edge removal, its 6 edges in the order of `Pass:320-333`: open ring by a physical face (`ring.surface`); open ring by
  a border face (`ring.border`); n < 3 or n > 7; `Blocked`; `Fits`; rim topology; the result of the dynamic program
  (long diagonal / not positive in floating point / topology, `Changes:368-370`); no rise (`:372`); `Prepare`; not
  exactly positive.
- Swap, its 4 faces in the order of `Pass:335-344`: physical face; border face (`neighbors < 0`); `Blocked`; long new
  edge p-q; `Fits`; `Prepare`; no rise (`:419`); not exactly positive.
- Move, its 4 points in id order: not free (surface / border); empty star; `Blocked`. Then for each step 1, 0.5, 0.25
  (`Changes:490-534`): no tensor sample; not positive in floating point; long new edge; no rise. Then not exactly
  positive. The reason of the move is the reason at its last tried step (the value that `MovePoint` returns).
The plan functions return the same default for several silent reasons, so test the silent conditions before you call
them. An option that passes every check is "accepted". That is possible only where the quality work stopped at its
limit: write the count.
Report, by class of 3.1: for each reason, the count of cells for which it is the ONLY kind of reason among all options
of that operation kind (edge removal / swap / move), and three counts: bad cells with no `Free()` point; of these, the
cells with 4 `surface` points; of these, the cells with no edge whose ring is closed (`Ring::Closed`, `Core.hpp:263`:
every edge lies on a physical or border face).
For the X runs, do 3.2 again at c4 of the last pass.

### 3.3 Short edges and the placement of surface points
- The 20 shortest edges by the metric length of the core (`Length`, `Measures:28-38`) at c3 of each pass (c4 for X
  runs), for each rank: metric length, length in space, the ids, coordinates, group and origin of both ends, on a
  physical face or not. Also the count of edges with metric length below 1e-6, 1e-4, 1e-2, 0.1.
- For each accepted surface split (from the recording wrapper): the ratios |a m| / |a b| and |m b| / |a b| in space, and
  the distance from m to the nearest vertex of its new cells other than a, b and m (the rim points), divided by
  |a b|. Report histograms (bins: below 0.01, 0.01 to 0.1, 0.1 to 0.25, 0.25 to 0.4, above 0.4) for sheet splits and for
  feature splits, and the 20 smallest values with the ends, m, the kind, and the step.

### 3.4 Stars
At each checkpoint: the histogram of the count of live cells at a point (bins 1-50, 51-100, 101-200, above 200) for
free points, surface points and border points. The 20 largest with id, coordinates, group and origin: only at c1 of
pass 1 and at c3 (c4 for X runs) of the last pass.
For runs with surface on, at c1: the count of free points whose star grew by more than 50 cells during the surface
steps, and for these points the sum of their growth split by the path of the splits that caused it (at each `Accept`,
each rim point gains half of the count of new cells at m that hold it): L; D by a face with the edge longest in both
faces; D by a face, longest in one; D by a face, longest in none; D by a feature chord (the classes of 3.5).

### 3.5 Which edge a surface split cuts
For each accepted surface split, from the two faces passed to `Place` (the mesh is not changed yet): is the edge the
longest edge (in space) of BOTH physical faces at it, of ONE, or of NONE. Counts by step (L, D by a face, D by a
feature chord).

### 3.6 Cells of each rank
For each rank at the end: cells, cells by class of 3.1, physical faces, and the bounding box of its physical faces.

## 4. Two pure volume operations, in test code (what can the volume work do without a change of the surface)

Both keep each physical face, each surface point and each border point unchanged. Mechanics, through the friend:
1. Stamp: at the start of each round, `++sweep_`. Use that value as the stamp of every change of the round. Skip a
   cell whose stamp already equals it.
2. V-split adds its point as `Split` does (`Changes:251-304`): a row `{{point_ids.Peek(), midpoint}, mean tensor}`,
   appended to `points_`. Use the `point_ids` of the pass, never your own sequence. Call `point_ids.Take()` only after
   acceptance. Remove the row again on any refusal.
3. Before any commit, with the trial row in place, check on the proposed vertex lists: `Fits(old, fresh, cell_limit)`;
   `Length(e) <= LongEdge` for each new edge; the worst `Margin` of the new cells > the worst `Margin` of the removed
   cells.
4. Only then call `ReplaceCells(old, fresh, stamp)`. It adds `Blocked`, the topology match of `Prepare` and the exact
   positivity test. Or call `Prepare` + `ExactlyPositive` + `Commit`, the same three steps.
5. No physical-face binding is necessary from you: a closed ring keeps each physical face whole in one child, and
   `Prepare` copies its binding.
Write each choice that this text leaves open in the report.

- Operation V-split (run X1): for a bad cell of class (a), (b) or (c), split one of its edges that has a closed ring
  (an edge on no physical face and no rank-border face, `Ring::Closed`, `Core.hpp:263`) at its midpoint in space
  (metric = mean of the ends, as `Split`), without the length condition of `Split`. Try the edges of the cell from the
  longest (in space) to the shortest; take the first that is accepted.
- Operation V-collapse (run X2): for a free point p of a bad cell of class (a), (b) or (c) that is next to the surface
  (`wall_near` after the `MarkWallNear()` of the round), and for each neighbour k of p (a surface point, a border point
  or a free point): build `fresh` as `CollapsePlan` does (`Changes:181-191`): the cells of the star of p without k get
  p replaced by k; the cells with k go away. k does not move. `Commit` marks p removed by itself (`Mesh:315-320`);
  do not change the row of p. Take the neighbour with the largest new worst margin
  among those that pass the checks of item 3; commit it by item 4; if the commit is refused, try the next best.
- Run X3: both; in a round, first the V-collapse sweep, then the V-split sweep.

Where: after c3 of each pass, as rounds. A round is: `MarkWallNear()`; the bad cells of classes (a), (b), (c) in the
order of `QualitySweep` (ascending `Margin`, then `Key`, `Pass:304-306`), skipping dead cells and cells changed in this
round; then `QualityWork(point_ids, cell_limit, stats)` (public). Repeat until a round accepts no V operation, at most
10 rounds. Checkpoint c4 is after the rounds. Then the pass ends as usual (`Check`, `MarkWallNear`, `ReportResult`,
`WriteBack`, exchange, shift).
Report for X1, X2, X3 (and X6), beside H1 (and H6) and beside H0 (and H6off): the table of 3.1 at the end of each pass
(c3 and c4), accepted operations by round, cells, the stars of 3.4, the short edges of 3.3, 3.2 at c4, exact
positivity of each cell at the end, and the seconds of the rounds. A cell that is not exactly positive in a run of
section 4 is a fault of the test code (R3): correct it.

## 5. Report and close

`R/night_20261115/FP2_B0/reports/FP2_B0_<build>.md` with the tables of sections 3 and 4, each correction of test code
(R3), and three lines at the top:
1. bad cells at hand-over (sum over the ranks, the last pass, c3; c4 for X runs) for H6off / H6 / X6 and for
   H0 / H1 / X1 / X2 / X3;
2. the three counts of 3.2 (no free point / 4 surface points / no closed-ring edge) for H6 (c3) and X6 (c4);
3. the smallest core metric length over all ranks at c3 of the last pass (c4 for X6) in H1, H6, H6off and X6, with the
   3.3 row of that edge.
Update the progress file, `HANDOFF_Codex.md` and `HowToProceed.md` (no change to `Handbook/ADAPTATION_TIMINGS.md`: no
production time changed). Commit ("FP2-B0 measure hand-over debt"), push, verify that no owned job runs, stop and wait
for the user.

## 6. Not in this item

A production change of any kind (a rule of the core, a print row of the solver, the engine). A full run. A change of
the tolerance, of `Limit()`, of the target (parked by the user). A surface collapse, swap or move.
