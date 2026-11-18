# Measurement M29: the mesh around a stuck cell (item N19)

Author: reviewer, for the main agent. Date: 2026-11-18. Base: `e1d9db1c17`.
This item is a measurement. It is print only. It changes no result, no gate and no limit.

## 1. Why

N16 C and N18 F failed with the same stuck cell (rank 3, cell 152845591, q = 0.198, limit 0.20).
The failure is exactly repeatable. M28 shows that each of the 14 features is blocked and that
each refusal is "progress" (index 3) or "geometry" (index 0).

The reviewer must design a general repair for stuck cells. For that design the reviewer needs
the real mesh around the stuck cell: the cells, the points and the metric. No file of the
failed run has these data. M29 prints them one time, when an event ends as not complete.

The reviewer will test the candidate repairs offline on these data before the design is written.
The data will also be the base of a unit fixture for the repair.

## 2. What does not change

- No production decision reads an M29 value (R11).
- M29 runs only when BOTH conditions are true: `options.timings`, and the event ended with
  `complete=0` (the value that each rank prints in the line `NATIVE3D complete=`).
- In a run that ends with `complete=1`, M29 executes no statement that communicates, no timer
  operation and no print. All gate runs are such runs. Thus G1, G2, G3, G4 and GQ stay identical.
- No field is added to `Session`, `CellState`, `RepairDemand` or a row that travels (R10).
  Each `NATIVE3D_TIMER_SUMMARY` row prints `fixedstorage=4120`.

## 3. Part A: the print (one commit)

### 3.1 When and where

Place the call at the earliest point where these three facts are true together:
(1) each rank knows that the event is not complete, (2) the final quality census of the stage
`exit` is done, (3) the private cells and their metric data still exist on each rank.
If no single point has the three facts, use the point that is nearest to the end of the stage
`exit` and write the choice in the questions file (R13).

### 3.2 The three levels

- Level 0: each stuck cell. "Stuck" has the M28 definition (`EngineReports::PrintStuckCells`):
  a cell with `q_bad` or `j_bad` whose repair features are all blocked. At most 8 for each rank,
  the same cells and the same order as M28.
- Level 1: each cell, on any rank, that contains at least one vertex of a level-0 cell.
- Level 2: each cell, on any rank, that contains at least one vertex of a level-1 cell.

A cell is printed one time, by the rank that holds it, with its lowest level.

### 3.3 Method (you select the accessors; this is the required behaviour)

1. Each rank collects the vertex ids of its level-0 cells. All ranks exchange these ids.
2. Each rank scans the cells that it holds. It prints each cell that contains one of these ids.
   It collects the vertex ids of the printed cells. All ranks exchange these ids.
3. Each rank scans its cells again. It prints each cell, not printed before, that contains one
   of the ids of step 2.

Use plain collectives for the two exchanges. Do not use the tracked exchange path of the rounds,
so that no round counter changes. The scans read data only.
Keep the functions short (D 7.1a): one function for each step, one function that prints a cell.

### 3.4 The lines (precision 17, decimal, one line for each cell)

```
NATIVE3D_M29_BEGIN rank=<r> round=<stats.rounds> stuck=<count of level-0 cells of this rank>
NATIVE3D_M29_CELL rank=<r> level=<0|1|2> cell=<id> surface=<0|1> q=<q> j=<J> debt=<q_debt + j_debt>
  lengths=<6 values> metric=<6 values> v=<4 ids> x=<12 values> m=<24 values> free=<4 flags>
NATIVE3D_M29_END rank=<r> level0=<n> level1=<n> level2=<n>
```

The line `NATIVE3D_M29_CELL` is ONE line in the output (it is shown on two lines here).

- `q`, `j`, `debt`: computed with the same functions as the quality census. Do not use a second formula.
- `lengths`: the six metric edge lengths that the census uses for this cell, in the edge order of the engine.
  Write the edge order (pairs of local vertex indexes) one time in the report.
- `metric`: the tensor that the engine used to measure `q` of this cell
  (order xx, xy, xz, yy, yz, zz).
- `v`: the four vertex ids in the stored order of the cell.
- `x`: x, y, z of the four vertices, in the order of `v`.
- `m`: the stored metric tensor of each vertex, 6 values for each vertex (order xx, xy, xz, yy, yz, zz),
  in the order of `v`. If the engine stores a tensor that is not the final metric (for example a
  sensor tensor that the composition changes), print the stored tensor and write in the report
  which function makes the final metric from it.
- `free`: for each vertex, 1 when a coordinate move of the repair ladder may move this vertex
  (interior, not constrained), else 0.

Each rank prints `NATIVE3D_M29_BEGIN` and `NATIVE3D_M29_END`, also with zero cells.

### 3.5 Gates of part A

Source check first (R4), in `R/night_20261115/N19/source_check.md`.
Build without a new warning. G1, G2, G3 (a, b), G4 and GQ identical, `fixedstorage=4120`.
No full run for part A. Commit and push.

## 4. Part B: one diagnostic run

Purpose: get the M29 lines of the known failure. This run is NOT a candidate for acceptance.

1. Apply again the three paths of the recorded candidate `6b56b42f16` (the production width 64).
   Use the commit pattern of N18 F: one commit with the label DIAGNOSTIC, later its revert.
2. Build. Run the full workflow with the same inputs, ranks and options as
   `round_8_5r2_F/G5/full580_run1`. The run stops after the first rejected event.
3. Check that the failure is the same (report each value):
   event 1 `complete=0`; rank 3 `q_bad=1`; work 2,268,665;
   the M28 line of stage `exit` names cell 152845591 with the same 14 feature entries as in
   `N18/reports/STEP_8_5R2_580.stuck.md`.
4. Revert the DIAGNOSTIC commit. Build the default binary again. Verify the source pins.
   Verify that no owned job runs.

No other gate applies to this run. No second run.

## 5. Failure rules

- Part A gate fails: R5. If it fails again, revert part A, write "N19 FAILED", stop.
- Part B: the failure is not the same as in N18 F: do not repeat. Keep all rank outputs.
  Write the first value that differs. The M29 lines are still the result.
- Part B: the M29 lines are missing or a level-1 cell set is not closed around a level-0 vertex:
  this is an error of your own print code (R3). Correct it one time, repeat part B one time.

## 6. Report

`R/night_20261115/N19/reports/M29_<build>.md` with:
- the gates of part A, the four checks of part B;
- the edge order and the metric note of section 3.4;
- for each rank the three counts of `NATIVE3D_M29_END`;
- the path of each rank output.

Also write `R/night_20261115/N19/reports/M29_dump.txt`: all `NATIVE3D_M29_` lines of all ranks
and the `NATIVE3D_M28_STUCK` lines of the stage `exit`, without the time prefix of mpirun.

Then update the progress file, commit, push, and stop. Wait for the user.

## 7. Not in this item

A repair of stuck cells, a change of the progress rule, another round width, a larger cavity.
The reviewer writes the repair design after the analysis of the M29 data.

## 8. Information from the user (no action)

On 2026-11-18 the user moved old untracked evidence files from the 4TB disk to
`/media/rausa/Elements2/SU2_NativeIntegrated_archive_20261118` (three tar files, each with a
`.list` file of its paths): `m6_runs/euler_build*` before build 290, `native_adaptation_partition_v1`
and `bl_remesh_v1`. Tracked files were not touched. The 4TB disk now has about 27 GB free.
