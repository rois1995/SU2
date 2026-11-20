# FP1 resume: correction 1 of work order 3, part C restored, part F (follow-up of the review), then the gates of part C

Author: reviewer, 2026-11-19, 23:30 (version 2, checked against the code of `5bf9b8b4fd`). For the main agent. The user
sends it; it is a user order.
Base: `a67515ff78` ("Close FP1 after OFF G2 reference mismatch"). All rules of section 1 of `AGENTS.md` apply.
R = `integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57`. "The text" = `FAST_PATH_STAGE1.md`
(SHA256 `3f195641c334c78f260cfdb140887615a8a776fcbfcc8182cc80e2d6f46a624f`, your copy in `R/night_20261115/FP1/inputs/`).
Copy this file with its SHA256 into the same folder.
Short names: NRR = `Common/src/adaptation/native_engine3d/NativeRemeshRun.inc`; Pass / Changes / Mesh / Shift = the files
`Common/src/adaptation/native_local/CNativeLocalCore3D<name>.cpp`; Hdr = `Common/include/adaptation/CNativeLocalCore3D.hpp`.
Line numbers are those of `5bf9b8b4fd`. "Part F" of this order is not the design D of R2.

Why: your stop of FP1 at the OFF G2 gate was correct by the letter. The letter was wrong: an error of the reviewer in
section 2a of work order 3. Your diagnosis showed that each compared row is equal to the accepted N22 part C capture. So
part C is not failed. This order corrects the rule, restores part C, adds points from the reviewer's code review of parts
A, B and C (no bug was found), and continues with the gates.

Do the sections in the order 1, 2, 3, 4.

## 1. Correction 1 of work order 3 (the rule for G2 after N22)

In section 2a of `AGENTS.md` (lines 132-133, the sentence goes over the line break), replace the sentence
"For G2 the `communication` calls of the preparation phase are those of N22 part C."
by:
"G2 and GQ use the comparator of N22 part C (`R/night_20261115/N22/compare_prefix.py` with part C). The calls and seconds
of each `communication` timer row (in each phase) and the collective, election and tracked-call counters of the integer
rows (the fields `C_FIELDS` of that comparator) are a report. All other rows are exact: integer rows, modeled bytes,
decisions, the timer rows of each other operation, `fixedstorage=4120`. Reference for the exact rows: N14 for G2, the N22
capture for GQ."

This is the rule under which N22 part C was accepted (`R/night_20261115/N22/C/G2_comparison.json`, `passed: true`). No
gate changes its meaning. A difference in ONLY these report rows (example: 333,799 against 340,843
`construction / communication` calls; 114,332 against 120,046 `collective_calls`) is not a failed gate and not a reason
for R5. Write the numbers in the report. Commit the change of `AGENTS.md` ("Work order 3, correction 1").

## 2. Restore part C

`git revert --no-commit d6e2ee8c4c`, then one commit "FP1 C restored by user order". The sources are then those of the
candidate `5bf9b8b4fd`. Open FP1 again in the progress file. Write in the report that the stop was caused by the
reference sentence, with the two commit ids (`5bf9b8b4fd` candidate, `d6e2ee8c4c` revert).

## 3. Part F: points from the review (before the gates of part C)

The review found no bug. F1 answers your question of 21:02. F2 is a speed rule with the same result. F3 is an R11
point. F4 makes the fixtures do real work.

### F1. Split sweep: defer by the ring (Pass, `SplitSweeps`, the lines with `stamped` and `touched`)
Meaning of "the same stamp rule" of section 7 step 3 of the text: the cavity of a split is its ring. New rule for an
entry of the split list, in this order:
1. its stored cell is not live, or has the stamp of this sweep, or does not hold both points of the edge: deferred;
2. else walk its ring; a ring cell has the stamp of this sweep: deferred;
3. else try the split.
Remove the per-point vector `stamped` and the list `touched`. (`Commit` gives each new cell the stamp, so rule 2 sees
each ring that an earlier split of this sweep changed.) This changes the ON mesh against `5bf9b8b4fd` (more splits in
one sweep); the counts in the prints of the fixtures change: report them (R3).

### F2. Memory of refusals (the same result, fewer evaluations)
Today each collapse sweep evaluates each refused short edge again, and each quality sweep evaluates each bad cell again,
also when nothing near it changed. Add:
- One change counter of the pass (`uint64_t`, starts at 1) and, for each point, `changed_at` (`uint64_t`, a member of
  `PointRow`, start value 0). `Mesh::Commit` (the one function that applies each topology change: collapse, split, edge
  removal, face swap and `ReplaceCells` all reach it) raises the counter by one and then sets `changed_at` to the new
  value for each point of each OLD cell and each NEW cell of the change. `MovePoint`, when it accepts a move, raises the
  counter by one and sets `changed_at` to the new value for each point of each cell around the moved point (the cells
  that it marks with `MarkChanged`, Changes:412).
- Collapse list: when `Collapse` (it evaluates both directions) returns a value other than `Refusal::None`, remember
  (the edge by its two point ids -> the counter value). Before an entry is evaluated: if it is remembered with a value
  that is not below `changed_at` of both its points, skip it and count it in a new print field `skipped_memory`.
- Quality list: when a bad cell gets no accepted option and no accepted move, store the counter value in its row
  (`failed_at`, `uint64_t`, a member of `CellRow`, 0 when a cell is made). Before a bad cell is evaluated: if `failed_at`
  is not zero and not below `changed_at` of its 4 points, skip it (`skipped_memory`).
- Do not remember a quality refusal when one of its options returned `Refusal::Memory` (`RemoveEdgePlan`,
  Changes:244-246; `SwapFacePlan`, Changes:324-327). Use a local flag of the entry, NOT the print counter
  `refused_memory` (R11). A collapse never returns `Memory`.
- A deferred entry is not a refusal and is not remembered. Split refusals are not remembered.
- Bytes: charge the collapse memory in `BytesPerCell` (Pass:43-47) with
  `6 * (2 * sizeof(Id) + sizeof(uint64_t) + 4 * sizeof(void*))`. When the collapse memory holds more than 6 entries for
  each live cell: clear it (this only removes skips). `BytesPerCell`, the cell limit and `bytes=` change by this and by
  the two new members; report the old and the new value (not a gate).
- The switch: add `bool memory = true` as the last parameter of `Mesh::RunPass` (Hdr:157). `RunPass` stores it in a
  member `memory_` (default `true`) that `CollapseSweeps` and `QualitySweep` read. NRR does not change for this. No
  config option.
- The memory is a lookup only: no decision iterates over it (no order from a hash container).
Reason (write it as the comment): the decision of an entry reads only: cells that hold one of its points; the
coordinates, tensors and ids of the points of those cells; the point flags, which are fixed in a pass, and the wall-near
mark, which depends only on the cells that hold the point; and the count of live cells, only through the cell limit
(refusals with that reason are not remembered). Each change of these raises `changed_at` of a point of the entry. So an
entry that is skipped would be refused again.

### F3. Shift: the admission must not depend on the print flag (Shift:109, :197)
Line 109: append to `rendezvous` also when `report` is false. Line 197: charge `GrowthBound(received.size(), sizeof(Id))`
always. Do not change the other uses of `report` in this file. Keep the parameter `report` of the functions, and write
a comment that the list is filled always because it is part of the admission. (No new collective call; nothing that is
printed under timings changes.)

### F4. Fixtures (R3; NEW test cases with new tags; the present tests and their boxes stay as they are)
1. A new box for part A (larger, or a stronger field) so that ONE pass makes each kind of change. Assert: collapses,
   splits, edge removals, face swaps, moves and `deferred` are each above zero. And one probe: on a copy of the start
   mesh after `MarkWallNear`, a short edge whose two ends are `Free()` and not `wall_near`, for which
   `Collapse(edge, stamp, stats, true)` returns `None` (a collapse where both ends can be removed).
2. Edge removal: for hand rings of 4, 5, 6 and 7 cells (`BoundMesh`, Operators tests; ring points that are not
   symmetric; one ring cell must be bad, so that the removal is accepted): the smallest `m` of the new cells after
   `RemoveEdge` is equal to the best value of a brute-force search over all triangulations of the ring with the rules
   of 6.4 (valid cells, diagonals not long, the value above the worst ring cell); when the brute force finds no
   accepted triangulation, `RemoveEdge` returns a refusal.
3. F2: `Mesh::RunPass` with the memory on and with the memory off, on the box of item 1 (on the present box if item 1
   did not fit): the same written-back `Owned` map (bytes) and equal `Points()` and `Cells()` arrays apart from the
   fields `changed_at` and `failed_at`; `skipped_memory` is above zero with the memory on.
4. Part B: the box of the part B test file with a field that the test states (for example the field of item 1 in the
   coordinates of that box), so that each pass makes collapses, splits and quality changes with a size of the mesh that
   fits R3. 2 ranks: slabs. 4 ranks: 2 x 2 blocks in (x, y) with the limits inside a cube (as `Slab(..., .625)`), so
   that points on three and on four ranks exist (assert both counts above zero). The full sequence (meeting, pass,
   shift "lower", meeting, pass, shift "higher"). Assert: summed over the ranks, each counter of the five changes of
   pass 1 is above zero; at least one cell that pass 1 wrote back holds a border point of meeting 1; `frozen_in_both`
   is above zero at 4 ranks and equal to the independent count; the union is valid and conforming; two runs give
   identical rows; the input of the engine check (fixture 12) has another count of cells than the box at the start,
   and the engine ends valid on it.
5. A wired case beside fixture 13: a larger mesh (`MakeSimplexMesh(3, n, ...)` with n at least 6; in the present case
   each free point is wall-near, so no collapse is possible) and a target that is not uniform, so that the wired run
   makes collapses and at least one quality change (sums over the ranks). `ADAP_NATIVE_REPARTITION= YES` at 2 and 4
   ranks (`NO` at 1 rank). Assert the rows `NATIVE3D_LOCAL_SHIFT`: for each direction, `sent` and `received` summed
   over the ranks are above zero at 2 and 4 ranks. Two runs give identical output.
Items 3 and 5 are necessary. If another item does not fit in the limit of R3: report it and continue.

### Gates of part F
Build without a new warning. The fixtures 1 to 14 of the text and the items of F4 pass. G1 identical.
`git diff 5bf9b8b4fd` over the code tree (`Common/`, `SU2_CFD/`, `UnitTests/`, the `meson.build` files): only the core
files of section 3 of the text, the function `PrintLocalPass` in NRR (the new field `skipped_memory=` after
`deferred=`), test files and `UnitTests/meson.build`. Documents (`AGENTS.md`, `HANDOFF_Codex.md`, `HowToProceed.md`,
`Handbook/`), the progress file and your evidence folders are allowed. Commit and push ("FP1 F").

## 4. Then: the gates of part C, as the text says

Sections 12.2 (OFF), 12.3 (ON, two runs directly after the OFF run), 12.4, 12.5 and 13 of the text, with the binary of
part F, with these additions:
- G2 and GQ by the rule of section 1 of this file. The G2 run of 22:25 is not used again (the binary changed).
- Text 12.2 item 1 reads: `git diff` of the part F commit to part B (`6dc801d609`) over the code tree: only the four
  `CConfig` places, the include, the call and the new functions in NRR (`RunLocalCore`, `PrintLocalPass`,
  `PrintLocalShift`), the core files, test files, `UnitTests/meson.build`. Documents, the progress file and evidence
  folders are allowed.
- Text 12.5, second point: a revert of part C reverts the restore commit and the NRR line of part F. The library and
  test changes of part F stay (they have no production call without part C).
- Report rows more. For each event, OFF beside ON: the process peak of each rank and the job peak;
  `Native3D frozen stages: count=`. ON only, for each rank and pass: `limit`, the three sweep counts, `rounds`,
  `skipped_memory`; the cells of each rank before the core (`cells_in` of pass 1), after shift "lower" and after shift
  "higher" (`cells_after` of the two shift rows).
- Watch points of the review for the ON runs (write what you see): event 2 memory (the shift holds two maps for a short
  time); a stall with no `NATIVE3D_LOCAL_SHIFT` row; a second normalization stage (then the core runs two times).

## 5. Not in this order

Any change of the engine, of a gate, a threshold or a default. A new config option. The items FX1 and FP2-0 (they come
with their own orders).
