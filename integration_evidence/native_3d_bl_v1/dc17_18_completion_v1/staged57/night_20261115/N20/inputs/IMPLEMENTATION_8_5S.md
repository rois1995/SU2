# Step 8.5s: the fine move for stuck cells (item N20), then the wide rounds again

Author: reviewer, for the main agent. Date: 2026-11-19. Base: `da20c9e9fe` (build 583).
All rules of section 1 of `AGENTS.md` apply (R1 to R13). No gate, budget or limit changes.

## 1. Why (facts; the reviewer tested them offline on the M29 data)

N16 C and N18 F failed because one cell stayed bad (rank 3, cell 152845591, q = 0.19806).
The reviewer rebuilt the 521 cells of `N19/reports/M29_dump.txt` offline. The offline measure
gives the printed q and J of all 521 cells (largest error 2e-15). Results:

1. No topology change repairs the cell: 12 collapses, all triangulations of 5 edge shells,
   4 face swaps, 6 edge splits (also with the new point at its best place). All refused,
   as in the engine. The limit of 16 apices is not the cause.
2. The three coordinate moves of the ladder (steps 1, 0.5, 0.25 to the centroid) are refused,
   with exactly the M28 notes (progress; for vertex 50267 also geometry).
3. A small move of ONE vertex repairs the cell. The move is 0.007 to 0.03 of the unit metric
   length. Vertex 200812891: debt 9.427e-05 -> 0, minimum score 0.9903 -> 1.0073.
4. The optimizer has a directed search (`PlaceCone`). It uses an approximate tensor, its
   steps start at 0.25, and its budget (160 evaluations for all starts) ends the search near
   step 0.03. The moves of fact 3 are below this resolution. This is the cause.
5. A compass search with the EXACT measure and finer steps (section 3.1) closes the cell in
   each tested variant: 1 to 4 accepted moves, about 200 evaluations for one search, also
   when the reviewer forbids one or two of the vertices, and for 24 random rotations of the
   direction set (each search was accepted).
6. The neighbourhood is tight: 24 of 29 free vertices have a star minimum score below 1.05.
   Such places are the normal end state of a hinge debt. The repair must thus be general.
7. The optimizer and the validator do not use the same tensor for a fresh cell. The optimizer
   queries the old field at the fresh centroid. `ValidatePrivate` uses the mean of the four
   nodal tensors, and the moved node gets the old-field tensor at its new place. Offline the
   two scores differ by 1e-4 or less for these moves. Each point of fact 5 has a margin of
   0.3 % or more (the search continues to raise the minimum score after the debt is 0), and
   the validator measure accepts each of them. Do not make the two measures equal in this item.

Offline scripts (information only): `/tmp/claude-1000/-media-rausa-4TB-SU2-Versions-SU2-NativeIntegrated/f3d988e5-f14f-4a96-b6fb-963ec11730e7/scratchpad/m29/`.

## 2. The idea, and what does not change

When the engine is about to stop with `OperatorStagnation` and stuck cells exist, it gives each
vertex of each stuck cell ONE more attempt: the optimizer rung with an exact, fine placement
search. Acceptance is unchanged. Then the normal rounds continue.

- "Stuck cell" = the M28 predicate (`BlockedShapeRepairs` in `EngineReports.cpp`): a cell with
  `q_bad` or `j_bad` whose repair features all have users and are all blocked.
- No change of: the progress rule (`ValidateMetricChange`, `MetricTrial`), the thresholds, the
  cavity limits (64 / 128), the work cap of the optimizer rung, the budgets, the selection,
  the ownership, the contact veto, the commit.
- A run that never stops with a stuck cell executes no new statement that changes a value.
  T0 has `stuck_shape=0` in each census. Thus part A is an item with identical results.
- No field is added to `Session`, `CellState`, `CachedRow`, a slot struct or a row that travels.
  `sizeof(RepairDemand)` stays 24 bytes (the existing `static_assert`). `fixedstorage=4120`.

## 3. Part A: the production change (commits A1 to A4; one commit for each is permitted)

### 3.1 A1: exact fine placement in the optimizer

File `Common/src/adaptation/native_engine3d/MeshOptimizer.cpp`, option in
`Common/include/adaptation/CNativeOptimize3D.hpp`.

- New option `bool fine_placement = false;` in `OptimizeOptions`, with a comment.
- In `MeshOptimizer::PlaceInteriorTargets`: when `options.fine_placement` is true, the placed
  point comes from the new function `FinePlacement` and not from the `PlaceCone` starts. All
  other statements of the function stay (projection `inPlane`, `++report.placements`, the
  insertion of the placed point as first target, the return value). The other targets (height,
  metric mean, hinge) stay. `CollectInteriorTrials` and `CommitInteriorSmooth` do not change:
  the fine point is tried as is, with the same exact merit, edge guard and interface check.
- New function `FinePlacement(p, old, x0, debt)`; it returns the best point and a flag.
  - Key of a point: first the star debt (lower is better; only when `debt` is true), then the
    minimum score of the star (higher is better). A point with a non-positive or unmeasurable
    cell is worse than each valid point.
  - The key comes from the per-cell function that `FreshMerit` uses (`Quality(cell, false)`,
    `NormalizedShapeHingeDebt`, `Score()`), on the star cells with `p` at the probe point.
    No second formula. Where `FreshMerit` returns `-INFINITY` for a requested-height failure,
    the probe is invalid.
  - Before the measure, each probe point passes through the same constraint that
    `CollectInteriorTrials` applies to a trial point (`ConstrainMovePoint` when the constraint
    patch is not empty, and `inPlane`). `CollectInteriorTrials` builds the constraint patch
    after `PlaceInteriorTargets`. Build it one time for the fine search with the same
    statements (one small shared function; the statements of `CollectInteriorTrials` stay
    the same).
  - Directions: the 14 directions of `PlaceCone` (6 axes, then 8 diagonals, same order), of
    unit metric length in the frame of the tensor `At(x0)`.
  - Search: `h = 1/16`. While `h >= 1/4096` and the count of evaluations is below 512:
    measure the 14 points `x + h * d`; go to the best of them when its key is better than
    the key of `x`; else halve `h`. Deterministic; strict comparisons; the first best wins.
  - Add the count of evaluations to `report.placement_evaluations`. Do not change
    `report.trials`.
  - Return "no point" when the best key is not better than the key of `x0`.
  - The numbers 1/16, 1/4096 and 512 are resolution limits, not tuned factors: the first is
    below the end resolution of `PlaceCone`; the offline result is the same with 1/1024 or
    1/16384 and with 256 or 1024 evaluations.
- The surface path (`SmoothBoundary`) does not change in this item (section 9).
- You may share the direction code with `PlaceCone` only if each arithmetic expression of
  `PlaceCone` stays the same. If not, write the 14 directions again with a comment.
- Pass the flag: one new parameter `bool fine_placement` of `OptimizedRepairPatch`
  (`CNativeEngine3D.cpp`) and of `TrialChecks::OptimizePatch`. Each existing caller passes false.

### 3.2 A2: the state of a demand and the request

Files `EngineSelection.hpp`, `EngineSelectionQueues.cpp`, `EngineReports.cpp`.

- Move the predicate `BlockedShapeRepairs` into `Selection` (a const member). M28, M29 and the
  new code use this one function.
- `RepairDemand` gets one `uint8_t` in its padding: the fine state `None`, `Requested`, `Done`
  (a small enum with a comment). The `static_assert` on the size stays.
- New function `Selection::RequestFineMoves()`: for each stuck cell of the census, for each of
  its vertex features (`count == 1`) whose state is `None`: set `Requested` and reopen the
  demand with the same statements that `TouchRepairSupport` uses to reopen one (not blocked,
  epoch, the two queues). It returns the count of reopened demands. It does not clear the
  M28 refusal notes.
- `TouchRepairSupport`: it sets the fine state of each demand of the touched support to `None`,
  blocked or not (the support changed; the normal ladder comes first again).
- `Selection::Service(feature, false)`: a demand in the state `Requested` becomes `Done`.
  Thus each rank that holds a copy of the demand gets the same state from the refusal receipt.
- A demand that is created new, or rebuilt after an ownership migration
  (`RestoreRepairState`), starts with `None`. A migration can thus permit one more fine attempt.
  The ownership cool-down limits the count of migrations.
- Why no copy loses its attempt: the request runs only when no rank has a candidate. Then each
  demand with users is blocked, each bad cell is stuck, and each rank that holds a demand of
  the vertex sets `Requested`.

### 3.3 A3: the stop

File `EngineControl.cpp`. Add ONE head decision `RoundHead::RequestFineRepair()` to
`RoundHead::Decide`, immediately before `CheckStagnationWindow`. The two places that set
`EngineStop::OperatorStagnation` do not change.

```
RequestFineRepair():
  if (any) return Proceed                      // a candidate exists
  if (not the quality repair of the improvement phase) return Proceed
  reopened = maximum over the ranks of (selection.RequestFineMoves() != 0)
  if (!reopened) return Proceed                // the stop comes as today
  choice = selection.Best(repairing); any = maximum over the ranks of choice.active
  return Proceed                               // the round continues with the reopened demands
```

- "Quality repair of the improvement phase": `repairing`, `phase == Phase::Improvement`,
  no `surface_target`, `!selection.geometry_repairs`, `!selection.size_repairs`,
  `!metric_cleanup`.
- Do not return `RoundDecision::Restart`: a restart measures the stagnation window two times
  at the same round and stops.
- The second pick uses the two statements of `PickRound` for `!coarsening`.
- For the first maximum use the passive path (as M29 does), so that no tracked collective
  count and no timer changes in a run that stops with no stuck cell.
- A reopened demand is a normal candidate for the stagnation window.

Termination: a demand goes back to `None` only after a commit changed its support (each repair
commit lowers the debt) or after a migration. Without these, each demand is tried one time,
then the engine stops as today.

### 3.4 A4: the ladder

File `EngineTrial.cpp`, `RepairLadders::Run`.
When the chosen repair feature has the state `Requested` on the rank that proposes
(the proposer chose it from its own `RepairBatch`; read the state from the local
`repair_users` map; do not add it to the choice or the proposal):
- do not run `MoveCentre` and `FallbackLadder` (the normal ladder refused the coordinate
  trials and the apex trials for this support);
- run `OptimizeRepair()` with `fine_placement = true`; all its guards stay
  (`!coordinate_only`, the work allowance, the donor import, `ValidatePrivate`,
  `NoteLadder(1, ...)`); the rest of `Run` stays.
The repair trials are private (local maximum and local vote), so one rank can skip rungs.
A refusal blocks the demand as today, and `Service` sets `Done` (section 3.2). An accepted
move gives the vertex a new identity; its old demands end.
A star above 64 cells, a surface vertex and a zero work allowance get no fine move; the demand
is refused and becomes `Done`.
If the source does not agree with a sentence of 3.3 or 3.4 and no clear reading exists (R4):
write it, do not implement A3 and A4, do A1, A2 and the fixtures F1 to F3, skip part W.

### 3.5 Print (R11, under `options.timings`)

- `NATIVE3D_FINE_REQUEST rank=<r> round=<n> stuck=<cells> reopened=<demands>` when a rank
  reopened at least one demand.
- `NATIVE3D_FINE_MOVE rank=<r> round=<n> vertex=<id> cells=<old cells> candidate=<0|1>
  accepted=<0|1> moves=<interior_moves> debt_reduction=<interior_debt>` after each fine attempt.
  Use only values that exist (no new field).

## 4. Fixtures (your test code, R3)

F1, real data, optimizer level. Build the two 10-cell stars of vertices 200812891 and 200812895
from `N19/reports/M29_dump.txt` (cells with these vertices; coordinates `x`, nodal tensors `m`).
Put the numbers into the test file as constants; keep the script that made them in the
evidence folder. The target is the linear field of the nodal tensors. Run `OptimizeMetricMesh`
on each star, all vertices of the star boundary fixed as in a repair cavity.
- `fine_placement = false`: write the result in the report (expected: no change). No assertion.
- `fine_placement = true`, star of 200812891: the star debt after is 0 and the minimum score
  is >= 1.003 (offline: 1.0073). Star of 200812895: the debt after is below 0.5 x the debt
  before (offline: 9.427e-05 -> 3.3e-06). Each: `placement_evaluations` <= 512 for one search.
F2, anisotropy. The star of 200812891 under the map x -> A x with A = diag(1, 1, 1000) and
the tensors A^-T M A^-1: the same assertions as F1 (q and J are invariant).
F3, no effect. A patch whose cells are all good, `fine_placement = true`: no move, and
`RequestFineMoves()` returns 0 on a selection with no stuck cell.
F4, state. A selection with one stuck cell: the first `RequestFineMoves()` reopens its four
vertex features; `Service(feature, false)` makes each `Done`; a second call reopens none;
`TouchRepairSupport` makes them `None` again.

## 5. Gates of part A (identical results)

1. Source check for the whole item first (R4): `R/night_20261115/N20/source_check.md`.
2. Build with no new warning. The fixtures F1 to F4 pass.
3. G1, G2, G3 (a, b), G4, GQ identical, `fixedstorage=4120`. The two new print families are
   report rows. The safeguards of `R/shared_engine_regression_gates.json`.
   (Reviewer check: the gate logs of N19 A have 3,781 census lines, each with `stuck_shape=0`.)
4. If a G1 record differs: R5. If the diagnosis shows a `NATIVE3D_FINE_REQUEST` in that case
   (a fixture that stagnated before): do not revert and do not change the reference. Write the
   case and both records in the report, skip part W, stop and wait for the user.
5. No full run for part A alone: no gate run has a stuck cell, and the first run of part W is
   the full run. If part W is not done, run G5 one time: six files byte-identical.
6. Commit and push.

## 6. Part W: the wide rounds in production (the mesh changes)

Only when part A passed. Apply again the three paths of the recorded candidate `6b56b42f16`,
one commit. The text and the gates are those of I8R2 section 5 and section 6 items 5 to 8
(G1 with the wide repeat case, G2, G4, G3 rule, safeguards; G5 run 1 with all standing gates,
the event-1 prefix identity against T0 and the isolated restart; G5 run 2 byte-identical to
run 1; the purpose rule with the confirmed ceiling 3288.032 s and the two T0 values
2236.870 s and 1416.499 s). T0 stays `round_8_5r_B/G5/full574`.
Run folder: `N4/section6_3_after_N11/round_8_5s_W/G5/`.
If part W passes: its six files are the new reference (rule of the user).

## 7. If a gate fails

- Part A: R5. If it fails again or the cause is in this text: revert part A, "N20 A FAILED".
- Part W, an event ends with `complete=0`: keep all rank outputs (they hold M28, M29 and the
  fine lines). No second attempt, no other width. Revert part W only, keep part A,
  "N20 W FAILED". Copy the `NATIVE3D_M29_`, `NATIVE3D_M28_STUCK` and `NATIVE3D_FINE_` lines of
  the failed event into `R/night_20261115/N20/reports/W_dump.txt`.
- Part W, another gate of items 5 to 7: R5, then the same revert.
- Part W, purpose rule only: revert part W only, keep part A, "N20 W FAILED its purpose rule".

## 8. Report

`R/night_20261115/N20/reports/STEP_8_5S_<build>.md` with: the gates; the fixtures with their
measured values; each `NATIVE3D_FINE_REQUEST` and `NATIVE3D_FINE_MOVE` line of the runs of
part W; for each stuck cell the rounds from its first M28 line to its repair; the table and
CSV of I8R2 section 8 for part W. Update `Handbook/ADAPTATION_TIMINGS.md` if part W passes.
Then the progress file, `HANDOFF_Codex.md`, `HowToProceed.md`, commit, push, stop, wait.

## 9. Not in this item

A fine move for a surface vertex (`SmoothBoundary`) or for a star above 64 cells; a change of
`PlaceCone` or of its budget for normal demands; a selection that avoids reservation
conflicts; another width; the lane 8.5ab. The reviewer decides these after the numbers.

## 10. Answers to the questions file

The four N19 entries (exit hook, edge order, mobility flag, print guard): each used option
is correct. No action.
