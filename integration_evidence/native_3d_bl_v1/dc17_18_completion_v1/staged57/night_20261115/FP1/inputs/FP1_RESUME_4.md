# FP1 resume 4: a split must make progress (the engine's rule), then the lab again

Author: reviewer, 2026-11-20, 13:45. For the main agent. The user sends it; it is a user order.
Base: `1391148436` ("FP1 H") with the lab evidence of resume 3. All rules of section 1 of `AGENTS.md` apply.
R = `integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57`. "The text" = `FAST_PATH_STAGE1.md`; resume 3 =
`FP1_RESUME_3.md` (your copies in `R/night_20261115/FP1/inputs/`). Copy this file with its SHA256 into the same folder.
Short names as before. Evidence: `R/night_20261115/FP1/resume4/`.

## Why (the sweep rows of your lab runs)

- LAB2 V2: 88.8 s for the pass, valid, 726,442 -> 577,406 cells, bad cells 22,541 -> 17,246. Good. But its split sweeps
  have a tail that grows (63, 69, 72, ... 117 splits in a sweep) until the limit of 50 sweeps.
- LAB1 V2: the first 10 split sweeps go down (10,357 -> 2,533 splits; about 70,000 in sum, close to the 82,000 splits of
  the engine in event 1). From sweep 11 the count GROWS in each sweep (2,230 -> 6,338) until the cell limit of the
  memory (1,127,484 cells). Then the collapse phase removes 138,673 points, and the split phase grows again.
Cause: a rule of the engine is missing in the core (an error of the reviewer's text, section 6.3: "No length test").
The engine accepts a split only when its longest new edge is shorter than the edge that it splits
(`CNativeMetric3D.cpp:504-527`, `frontier_progress`: `longest_added < longest_removed * (1 - 1e-12)`; the text of its
refusal: "size insertion does not shorten the longest changed-edge frontier"). Without this rule a split can make an
edge that is longer than the one it removed, and the splits feed themselves.

## 0. End the lab campaign of resume 3

This order allows you to END the lab runs of resume 3 that still run or wait (V3, V1, V0); their question is answered
by the rows above. Keep the receipts of the two V2 runs. Commit the lab evidence as it is ("FP1 H lab, part").

## 1. Part H6: the progress rule of a split (correction of section 6.3 of the text)

In `Split`, in the trial (float numbers), before `Prepare`: with `L_old` = the length of the edge `ab` that is split,
and the new edges = the two halves `(a, m)`, `(m, b)` and the edge `(m, r)` for each ring point `r`: the split is
refused with `Refusal::Size` (counted in `refused_size`) when the longest new edge is not below
`L_old * (1 - 1e-12)`. Use the tensor of the new point (the mean of the two ends) for these lengths.
- One more switch in `CoreOptions`: `bool split_progress = true;` (false gives the core of part H).
- A refused split is not remembered (as today). Inside one call of `SplitSweeps` it stays refused while its ring does
  not change, so the work list of H5 stays exact; the fixture of H5 must pass with `split_progress` on.
- The sort of the split list does not change (longest first).
Reason (comment): the removed edge of a split is the split edge alone; when each new edge is shorter than it, the
longest edge near the change goes down in each accepted split, so the splits can not feed themselves. A long edge whose
split is refused stays for a later sweep (after its longer neighbours were split) or for the engine.

## 2. Fixtures and gates of part H6

1. Fixture: a hand ring where the edge from the midpoint to one ring point is longer than the split edge: refused with
   `Size`, arrays bit-equal; the same ring with that ring point nearer: accepted.
2. Fixture: the H5 fixture (work list on and off: the same bytes) with `split_progress` on, on both boxes.
3. All earlier fixtures pass (counts that change: report, R3). In fixture 4 of the text, a long edge with a closed ring
   that is left must be one that `Split` refuses (`Size` by this rule, `Validity`, or the cell limit) or a blocked one.
4. Build without a new warning. G1 identical. G2 and GQ (OFF check). `git diff 1391148436` over the code tree: only the
   core files, the print functions of NRR if a counter is added, test files, `UnitTests/meson.build`.
Commit and push ("FP1 H6").

## 3. The lab again (as section 7 of resume 3; each run its own process with the 20 minute limit)

| Variant | Switches |
|---|---|
| V2 | all defaults (with `split_progress` on) |
| V5 | as V2 with `surface_first` off (does the progress rule alone stop the growth at a coarse surface?) |
| V3 | as V2 with `short_edge` = 0.9 / sqrt(2) |
Order of the runs: V2 on LAB1, V2 on LAB2, V5 on LAB1, V3 on LAB2. The same report as resume 3 section 7, and for each
run one more table from the sweep rows: for each kind and round the sweeps, the seconds, the accepted changes and the
live cells at its end.

## 4. Then

The stop rules of section 9 of resume 3 hold again, from its item 1 (the lab numbers of V2 after this order), with the
commit of part H6 in the place of the commit of part H.

## 5. Not in this order

A change of the thresholds, of the sweep limits or of the rounds. A change of the engine. FX1, FP2-0.
