# FP1 resume 3: surface first as a data rule, no new flat cells, float trials, a work list, a row for each sweep, and a lab test before any long run

Author: reviewer, 2026-11-20, 13:00 (version 2: checked against the working tree with part G; with the rows of the replay
of resume 2). For the main agent. The user sends it; it is a user order.
Base: the state after resume 2 (part G). All rules of section 1 of `AGENTS.md` apply.
R = `integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57`. "The text" = `FAST_PATH_STAGE1.md`; resume 1 =
`FP1_RESUME.md`; resume 2 = `FP1_RESUME_2.md` (your copies in `R/night_20261115/FP1/inputs/`). Copy this file with its
SHA256 into the same folder. Short names as before (NRR, Pass, Changes, Mesh, Measures, Shift, Hdr). Line numbers are
those of the working tree with part G. Evidence: `R/night_20261115/FP1/resume3/`.

## Why (the rows of your replay, pass 1 of event 1)

| | Ranks 0, 1, 4, 5 | Rank 2 | Rank 3 |
|---|---|---|---|
| Cells in -> out | about 50,000 -> about 50,000 | 56,223 -> 330,907 | 51,202 -> 1,112,091 |
| Bad cells in -> out | 1,074 to 5,917 -> 335 to 1,285 | 572 -> 209,661 | 0 -> 846,177 |
| Seconds collapse / split / quality | 11 to 27 / 15 to 19 / 1 to 3 | 148 / 126 / 1,061 | 308 / 85 / 3,677 |
| Sweeps collapse / split | 82 to 147 / 169 to 221 | 209 / 400 | 155 / 96 |

Four errors of the design of the reviewer follow from these rows:
1. **Volume before surface.** Ranks 2 and 3 hold the wing (84,534 wall-near points on rank 3). There the surface is too
   coarse for the target and the core can not split it. Each interior edge to a surface point stays long, is split
   again, and the layer at the wall fills with flat cells. This is against the rule "surface first" of the user.
2. **Flat cells from the collapse** (no shape number is read), then a quality work that does not converge there
   (158,166 changes in sweep 1, still 85,075 in sweep 20).
3. **Exact tests in trials**: 88,931,886 refusals for validity on rank 3, each with the exact `Orientation`.
4. **A full scan for each sweep**: in the calm ranks the collapse and split time (26 to 46 s) is mostly 250 to 370 scans
   of all edges; the changes themselves are few.
Also seen: after each split phase a large collapse phase (rank 0: s5500, c4115, s1468, c1495). It converges in 6 to 8
rounds. Section 5 measures a variant with a lower collapse threshold; the default does not change.
The user decided on 2026-11-20: in the new core a collapse may read the shape (an exception for the core only).

Order of work: sections 0 to 8. The lab test (section 6) decides how far this order goes (section 7).

## 0. Close resume 2

This order allows you to END the running replay of resume 2 now (its result is known; R12 does not hold for it).
Commit part G ("FP1 G"): exactly the part G files (Hdr, Changes, Pass, NRR, `UnitTests/meson.build`,
`CNativeLocalCore3DResumeMove_tests.cpp`) and your evidence of resume 2; not the `.orig` files and not the `externals`
type changes of the working tree. Gates of resume 2 section 5 items 1 to 3. Then continue here.

## 1. The switches of the core (for the lab test and the fixtures; no config option)

One public struct `CoreOptions { bool memory = true; bool shape_test = true; bool float_trials = true;
bool surface_first = true; bool work_list = true; double short_edge = ShortEdge; }`. `Mesh::RunPass` takes it as its
last argument (default `{}`) and stores it in a member; the functions that the fixtures call directly use the stored
value. NRR passes nothing (all defaults). With `shape_test`, `float_trials`, `surface_first`, `work_list` all false the
core is the part G core: show it in a fixture (the box pass of resume 1 item F4.1 gives the same counters and the same
`Owned` bytes as the part G binary printed).

## 2. Part H1 (`shape_test`): the collapse does not make a new bad cell

This corrects the first paragraph of section 6 of the text ("no q / J number is read" now holds for the split only) and
the list "Refused when" of 6.2.
In the evaluation of one direction of a collapse (`CollapsePlan`; remove `a`, keep `b`), after the present checks:
`w_new` = the smallest margin of the new cells (`plan.fresh`); if `w_new < BadMargin`: `w_old` = the smallest margin of
the old cells (`plan.old`, all cells around `a`); the direction is refused with a new reason `Refusal::Shape` (print
counter `refused_shape`; add its case in `PassStats::Count`) when also `w_new < w_old`.
The choice between two accepted directions does not change. The memory of refusals treats `Shape` as each other
refusal. If H1 makes a pinned hand collapse of an earlier fixture refuse: report it and change only the input mesh of
that fixture, not its assertion.

## 3. Part H2 (`float_trials`): trials in float, the exact test only for a change that is applied

This corrects the row "Validity of a cell" of section 5 of the text: in a trial, valid = float volume above zero;
before a change is applied and in the self-check, valid = exact `Orientation` positive.
- Add a private `bool Mesh::FloatPositive(Vertices) const` in Measures beside `Positive`: `FloatOrientation` of the 4
  points in the given order is above zero and finite (the same expression and order as the margin uses).
- Trials use `FloatPositive`: the plan check in `Prepare` (Mesh:191), the recursion of the edge removal (Changes:297),
  the star test of `MovePoint` (Changes:424).
- The exact check before a change is applied: one private const function `bool Mesh::ExactlyPositive(const Plan&) const`
  (the exact `Positive` of each cell of `plan.fresh`). Call it: (1) in `Collapse` for the chosen direction, after the
  choice and before `if (apply)` (so a call with `apply = false` answers as the sweep does); (2) in `Split` after
  `Prepare` accepts and before `free_points_.pop_back()` and `point_ids.Take()`; on refusal restore the point row as the
  present refusal path does; (3) in `QualitySweep` before `Commit(best, ...)`; (4) in `RemoveEdge` and `SwapFace` before
  `Commit`; (5) in `ReplaceCells` before `Commit` (it returns `Validity`). `MovePoint`: after the float trial accepts a
  step, write the candidate row with the old id and test the exact `Positive` of each cell around the point; on refusal
  restore the row; take the new id only after the exact test passed. `Commit` itself does not change. No id is taken
  and the change counter is not raised before the exact check passed.
- An exact refusal ends the entry: for a collapse the other direction is not applied; for a quality entry no other
  option, no move after it, no other step and no other point (`MovePoint` reports it with an output argument). Count it
  one time in `refused_valid` and one time in a new print counter `refused_exact`. The memory of refusals remembers it.
- Naming orders from a live cell (Changes:134, :340): do not call `Orientation`. A live cell is exactly positive, so
  the sign of a permutation of its points is the sign of the permutation. One small function (proposal
  `bool EvenOrder(const Vertices& cell, Vertices order)`).
- The exact `Positive` stays in: the input check when the arrays are built (Mesh:100), `ExactlyPositive`, the move
  check above, the self-check (Mesh:314).
- The tensor of a moved point: a float sampler, public in Hdr beside `SampleMovedTensor`:
  `bool SampleMovedTensorFloat(const std::vector<DonorCell>& donors, Point point, Tensor& tensor, size_t* chosen = nullptr)`.
  One loop over `donors` (the list that `MovePoint` builds today: old coordinates): the weights of `point` in the cell
  by Cramer's rule in `double`; a cell whose determinant is not above zero or not finite, or that gives a weight that is
  not finite, is skipped; the cell with the largest smallest weight wins (equal: the smaller `DonorCell::Key`); its
  negative weights are set to zero and the weights are scaled to the sum 1; tensor = the sum over k = 0..3 of weight
  times nodal tensor. No cell left: false (the step is refused with `Numeric`). With `float_trials` false the core uses
  `SampleMovedTensor` as part G does.

## 4. Part H4 (`surface_first`): the core does not work where the surface is not at its target

- "Unresolved surface point" = a point with the flag `surface` that is an end of a surface edge with `L > S` (a surface
  edge is an edge of a face with neighbour `-1`). The surface is frozen, so this mark is fixed for the pass. With more
  than one rank the meeting exchange (section 9 of the text) carries one more mark for a border point, "a rank has this
  point as unresolved", and returns the OR, as it does for the surface mark.
- "Blocked cell" = a live cell that has an unresolved point.
- No change of the core has a blocked cell in its cavity: collapse (the cells around the removed point), split (the
  ring), edge removal (the ring), face swap (the two cells), move (the cells around the point). Such an entry is not a
  candidate; count them in a print counter `blocked` (one count for each entry that a list drops for this reason).
- A cell made by a change is blocked by the same rule (it has an unresolved point or not).
Reason (comment): the volume at a surface that is too coarse can not reach its target before the surface does; the
engine does that region (stage 2 gives the core the surface and removes this rule). The wall-near rule stays.

## 5. Part H5 (`work_list`): the candidates of a sweep without a full scan (the same result)

In `CollapseSweeps` and `SplitSweeps`: the first sweep of a call lists its candidates by the full scan, as today. Each
later sweep of the same call lists only:
(a) the entries that the sweep before deferred; and
(b) each edge with an end point `p` whose `changed_at` is above the value of the change counter at the start of the
    sweep before (find them from the cells that the sweep before made: their points, the cells around these points,
    their edges);
then the same candidate conditions, the same sort and the same loop as today.
Reason (comment): an edge that is in neither set was refused or skipped in the sweep before and nothing near it changed;
with the memory of refusals the full scan would skip it again; a refused split stays refused inside one call (the cells
only grow in number there and its ring did not change). So the accepted changes and their order are the same.
`work_list` needs `memory` (with `memory` false the core scans). Print counters (`evaluated_*`, `skipped_memory`,
`refused_*`) can differ from the full scan; decisions and results do not.
Fixture (necessary): the pass on the box of resume 1 item F4.1 and on the new box of 8.1(e) with `work_list` on and off:
the same written-back `Owned` bytes and the same counts of the five changes. If this fixture fails and the cause is not
a coding error: set the default of `work_list` to false, report it, and continue (this fallback is part of the order).

## 6. Part H3: one row for each sweep, printed at once (print-only, R11)

A member `std::function<void(const SweepRow&)> sink` of `PassStats` (called only when `report` is true). After each
sweep the core calls it with: kind (collapse, split, quality), `round` (= `stats.rounds`; 0 for a quality sweep),
`sweep` (the pass counter of that kind after its increment), candidates listed, and the change of each pass counter
during the sweep (evaluated, deferred, skipped by memory, blocked, accepted: collapses, splits, edge removals, face
swaps, moves; the refusals by reason, counted for each evaluated option as the pass counters are), live cells, bad
cells listed (quality sweep), seconds of the sweep. The sums of the rows are the pass counters.
NRR gives a sink that prints `NATIVE3D_LOCAL_SWEEP rank= stage= pass= kind= round= sweep= ...` with `std::endl`, so that
a run that is cut keeps its rows. The row `NATIVE3D_LOCAL_CORE` gets the new counters (`refused_shape`, `refused_exact`,
`blocked`).

## 7. The lab test (test code; one rank; each run under a wall limit)

A new hidden test (`[.][NativeLocalCoreLab]`) in the `su2_cfd_tests` list. Data, selected by environment variables:
- LAB1 (input of event 1): mesh `G5/m6.su2` with `G5/m6.su2.native_ref`, tensors `R/night_20261115/FP0/cases/B/sensor.bin`;
- LAB2 (input of event 2): mesh `G5/adapted_mesh_adap_00001.su2` with its `.native_ref`, tensors
  `R/night_20261115/FP0/cases/A/sensor.bin`; G5 = `R/night_20261115/N4/section6_3_after_N11/round_8_5t_P/G5/full587`.
Loading: copy the loader of FP0 (`LoadMesh`, `CNativeFastPathStage0_tests.cpp:107-139`) into the lab test file; drop its
fixed counts and its `EntityGeometry` requirement (the volume map needs neither; the FP0 file does not change).
`ImportGeometry` on one rank gives the `Owned` map with tensors. Then the core alone: arrays from `Owned`,
`Mesh::RunPass` with a sink that prints each sweep row at once, write back.
Run each pair (data set, variant) as its own test process from your runner script, with a wall limit of 20 minutes
(`timeout`); at the limit the runner ends that process (your own test process), its printed rows are the result, and
the runner starts the next pair. One process at a time (R1).
Variants (the switches of section 1):
| Variant | shape_test | float_trials | surface_first | work_list | short_edge |
|---|---|---|---|---|---|
| V0 (part G) | off | off | off | off | 0.9 |
| V1 | on | on | off | on | 0.9 |
| V2 (the production form after this order) | on | on | on | on | 0.9 |
| V3 | on | on | on | on | 0.9 / sqrt(2) |
Order of the runs: V2 on LAB2, V2 on LAB1, V3 on LAB2, V1 on LAB1, V1 on LAB2, V0 on LAB2 (V0 may not end; that is its result).
Report for each run: each sweep row; "the pass seconds" = the wall seconds from before the `Mesh` constructor to after
the write-back (not the loading); cells, points, bad cells, short edges, long edges before and after; the counts of
each change, each refusal, `blocked`; then on the written-back map: each cell with exact `Orientation` positive, and the
strict `MeasureMetricCells` counts of q-bad, J-bad and long-edge cells.

## 8. Fixtures and gates of the code parts (before the lab runs)

1. Fixtures (new test cases, new tags):
   (a) H1: a hand mesh where a collapse is valid and short enough but makes a flat cell from good cells: refused with
       `Shape`; the same when the old cells are worse than the new: accepted.
   (b) H2: the box pass with float trials gives a valid mesh (self-check, `MeasureMetricCells`); a hand case, made
       through `Split` (a nearly flat ring cell, the rounding of the midpoint crosses the plane; search the coordinates
       in the test), where the float volume of a new cell is above zero and the exact `Orientation` is not positive:
       the change is not applied, `refused_exact` is 1, the arrays are bit-equal to before; `EvenOrder` equals the exact
       `Positive` for each live cell of the box and each of the 24 orders.
   (c) the float sampler against `EvaluateCavitySample`: on stars where the best smallest weight is above the second
       best by more than 1e-9, the same cell; on all stars each tensor component within 1e-12 times the largest absolute
       component.
   (d) H4: a box with one surface edge that is long: no change has a cell with an end of that edge in its cavity
       (compare the cells that touch those points before and after: bit-equal); `blocked` above zero; with 2 ranks a
       border point that is unresolved on the other rank only is blocked on both.
   (e) H5: the fixture of section 5, on the box of F4.1 and on a second box whose length work needs at least 5 sweeps
       of each kind in one call.
   (f) H3: the sink gets one row for each sweep and the sums of the rows are the pass counters.
   (g) the fixture of section 1 (all switches off = part G).
2. All earlier fixtures pass. In fixture 4 of the text "accepts" means `Collapse(edge, stamp, stats, false)` returns
   `None` with H1, H4 and the exact check; the split probe accepts `Validity` from the trial or from the exact check.
   Counts that change by the new rules: report them (R3).
3. Build without a new warning. G1 identical. `git diff` to the commit of part G over the code tree: only the core files;
   in NRR the print functions and, in `RunLocalCore`, the sink and the meeting mark of H4; test files; `UnitTests/meson.build`.
4. G2 and GQ by the rule of resume 1 section 1 (OFF check of the new binary; use these results again in 9.3).
Commit and push ("FP1 H") before the lab runs; a second commit for the lab results.

## 9. How far this order goes (stop rules; they are not gates)

1. After the lab: if the pass of V2 on LAB1 or on LAB2 needs more than 600 s, or its result is not valid (a cell that
   is not exactly positive, or a failed self-check): STOP. Commit, report, wait for the user.
2. Else: the replay of event 1 at 6 ranks with the core ON (as section 4 of resume 2, with the sweep rows). "Core
   seconds of a rank" = the sum of `seconds=` of its `NATIVE3D_LOCAL_CORE` rows and its `NATIVE3D_LOCAL_SHIFT` rows of
   stage 0. If the core fails, or no `NATIVE3D_LOCAL_SHIFT direction=higher` row of each rank is printed 600 s after the
   core started (then end your replay process; this order allows it), or the core seconds of a rank are above 180 s, or
   the event is not accepted: STOP, report (with the same report as resume 2 section 4), wait.
3. Else: the gates of part C as resume 1 section 4 says (12.2 to 12.5 and 13 of the text), with these readings: the OFF
   proof is the short gates (G1, G2, GQ of 8.2 to 8.4; G3; the safeguards) on the new binary plus the G5 OFF run of
   build608 (show that the OFF path has no changed statement since `20f425e3e6`: `git diff 20f425e3e6` over NRR outside
   `RunLocalCore` and the print functions, and over `CConfig`, is empty); then the two G5 ON runs one after the other;
   the time rule uses the OFF times of build608 (1,818.319 / 1,039.725 s) and a miss is "OPEN for the user"; in 12.2
   item 1 the commit is the one of part H.

## 10. Not in this order

A change of the default thresholds 0.9 / 1.8 or of the sweep limits (V3 is a lab variant only). A change of the
engine. Surface changes in the core. FX1, FP2-0.
