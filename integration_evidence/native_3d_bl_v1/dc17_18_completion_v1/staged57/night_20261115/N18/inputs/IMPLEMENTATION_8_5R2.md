# Step 8.5r2: corrected retry of the larger quality rounds, with measurement M28 (item N18)

Author: reviewer, for the main agent. Written 2026-11-18 after the failure of N16 part C.
Base: the tree after N6 (parts A and B of N16 are in the tree, part C is reverted by `3276cbb862`).
All rules R1 to R13 of work order 2 apply. Names are given, not line numbers, because N6 moves code.
Line numbers below are those of commit `3276cbb862` and are only a help to find the place.

## 1. Why (facts from the failed run `round_8_5r_C/G5/full575_run1`, checked by the reviewer)

1. The wide run stopped in event 1 with one q-bad interior cell on rank 3 (q_debt 9.427e-05).
   All 14 repair features of this cell (4 nodes, 6 edges, 4 faces) were `blocked`. A blocked repair
   feature is released only by `Selection::TouchRepairSupport`, that is, when a tetrahedron that
   contains the whole feature changes. The neighbours of the cell were good cells with no repair
   demand. So nothing could release the cell. `StartGeometryRepair` needs zero q-bad cells, so the
   control went to metric cleanup (1,946 rounds, zero commits) and then to `StopWithoutCandidate`.
   The logs do not name the cell or the reasons of the 14 refusals. M28 (part E) prints them.
2. Error of the reviewer in I8R item B9: `EngineOwnership::PhaseOwnership` used
   `WindowRounds(options, quality)` for its cool-down. With the wide batch this is 8 rounds. The
   quality ownership evaluation ran 90 times on each rank (T0: 33 times). The operation
   quality/scheduling went from 151.8 s to 515.4 s (sum of six ranks). Each evaluation that does not
   migrate adds its time to `stats.ownership_seconds`, which is a term of the predicted cost.
   Part D corrects this.
3. The wide rounds lost half of their candidates: `batch_candidates` 84,900, `batch_reserved` 42,740
   (T0: 43,999 and 39,166); `conflicts` 43,848 (T0: 5,629). The logs do not say if a candidate lost
   to a candidate of its own rank or of another rank. M28 (part E) counts this. It is the input of a
   later design. It changes nothing in this item.
4. Measured in the failed event 1 (not accepted): native maximum 1,958.8 s (T0: 2,236.9 s); quality
   rounds 716 for each rank (T0: 1,274); quality join wait 1,875.7 s (T0: 3,213.6 s); quality
   communication 2,247.9 s (T0: 1,833.3 s). Sums are over the six ranks.

This item has one aim with two possible useful results. If the corrected wide round gives an
accepted mesh two times, it is the new reference and the adaptation is faster. If a cell is stuck
again, the M28 lines give the data for the design of the repair, and part F is reverted.
There is no second attempt and no change of the value 64 (R2, R5).

## 2. What does not change

No gate, budget, tolerance or limit. `MaximumRepairBatch = 64`, `MaximumCandidateBatch`,
`MaximumDirectoryFeatures`, `MaximumDirectoryScan`, `MaximumCavityCells`, `MaximumReplacementCells`
keep their values. `RoundHead::CheckStagnationWindow` keeps `WindowRounds(options, repairing)`.
The selection rule, the reservation rule, the block rule and the release rule do not change.
With `repair_batch == 0` all results are identical to T0 (build574).

## 3. Part D: correction of the ownership cool-down (identical results)

File `Common/src/adaptation/native_engine3d/EngineOwnership.cpp`, function
`EngineOwnership::PhaseOwnership`, the early return (line 36 to 38):

```cpp
  if (ownership_evaluated[boundary] && (!quality || stats.rounds < quality_ownership_round ||
                                        stats.rounds - quality_ownership_round < options.stagnation_window))
    return true;
```

Only the last operand changes: `WindowRounds(options, quality)` becomes `options.stagnation_window`.
Add one comment line above it:
`// One evaluation has a fixed cost, so its cool-down counts rounds and does not scale with the batch.`

Reason for "identical results": `WindowRounds` returns `options.stagnation_window` when
`repair_batch == 0`. So the value is the same for every run of today.
If `WindowRounds` then has no other reader than `RoundHead::CheckStagnationWindow`, it stays as it is.

## 4. Part E: measurement M28 (print only, R11; identical results)

All code of part E runs only under `options.timings`. No decision reads its values.

### E1. Refusal notes of a repair feature

In `Selection` (`Common/src/adaptation/CNativeEngine3D.cpp`, or its place after N6):

```cpp
  // users: cells that ask for this repair. epoch: last service. blocked: last trial was refused.
  // refusals (print only, M28): for each ladder kind, the bits of the rejection indexes seen
  // since the support of the feature last changed. It uses padding bytes; the size is unchanged.
  struct RepairDemand { size_t users=0;uint64_t epoch=0;bool blocked=false;std::array<uint8_t,3> refusals{}; };
  static_assert(sizeof(RepairDemand) == 3 * sizeof(uint64_t), "M28 must not change the modeled repair bytes.");
```

If the `static_assert` fails on this compiler: do not implement E1 and E2 (R4), do E3 only.
If the two existing brace initializations of `RepairDemand` (three values) give a new warning, add
`{}` as the fourth value. Nothing else changes there.

New method of `Selection`:

```cpp
  // M28, print only: records that ladder kind `kind` refused this feature with rejection index `index`.
  void NoteRepairRefusal(const Feature& feature, unsigned kind, unsigned index) {
    const auto found = repair_users.find(RepairFeature{feature.count, feature.vertices});
    if (found != repair_users.end() && kind < 3 && index < 8) found->second.refusals[kind] |= uint8_t(1u << index);
  }
```

Clear the notes where the support changes: in `Selection::TouchRepairSupport`, in the statement that
sets `demand.blocked=false`, add `demand.refusals={};`.
A new entry starts with zero notes (the default member value).
The migration record (`repair_epochs`, `repair_active`, `repair_blocked`) does not change. After a
migration the notes of a moved feature are zero. This is accepted and is visible in the print.

Writer: `TrialChecks::NoteLadder` (`EngineTrial.cpp`). In its `else` branch (the trial is not valid),
after the six existing `counter.rejected[...]` statements, add under `options.timings`: for each of
the indexes 0, 1, 2, 3, 4, 7 whose existing condition is true, call
`selection.NoteRepairRefusal(feature, kind, index)`. `feature` is the repair feature of the live slot,
the same value that `RepairBatchCommit::FinishProposalRouting` later sends as
`proposal.choice.feature`. Do not evaluate the conditions two times in a different way: compute the
six booleans one time into local `const bool` values and use them for the counters and for the notes.
The counter values must stay identical.

The two other rejection indexes have their own writer, under `options.timings`:
- index 5 (contact veto): in `RepairBatchCommit::CheckContact`, at the existing statement
  `++stats.quality.ladder[proposal.ladder].rejected[5];`, add
  `selection.NoteRepairRefusal(proposal.choice.feature, proposal.ladder, 5);`.
- A refusal before the trial (`RoundAcquisition::Refuse`, oversized star) in a repair round:
  after `selection.Block(...)`, add `if (options.timings && repairing)
  selection.NoteRepairRefusal(feature, 2, 6);`. In this print, kind 2 with index 6 means
  "refused before the trial (oversized star)". Index 6 has no other writer for the notes
  (a lost final reservation does not block).

The order "note, then `Service(feature,false)`" or the opposite order does not matter:
`Service` does not touch the notes.

### E2. The print of the stuck cells

New function in `EngineReports` (keep it under 40 lines; one helper is allowed):

```cpp
// M28, print only: names each bad cell whose repair features are all blocked, with the refusal notes.
static void PrintStuckCells(EngineContext& context, <type of the stage argument of PrintQualityCensus> stage);
```

Call it in the quality census printer (the function that calls `CollectQualityCensus` and then
`PrintQualityCensus`), directly after `PrintQualityCensus(...)`, only when `stuck_shape != 0`.
This printer already runs only when the quality rows are printed. Do not change
`CollectQualityCensus`.

A cell is printed when `row.debt.q_bad || row.debt.j_bad`, its entry in `selection.cell_repairs`
exists and is not empty, and every key of it has an entry in `selection.repair_users` with
`users != 0` and `blocked == true` (the same test as in `CollectQualityCensus`).
Print at most 8 cells for each call and rank, in the order of `selection.cells`. One line for a cell:

```
NATIVE3D_M28_STUCK rank=<r> stage=<stage> round=<stats.rounds> cell=<id> surface=<row.surface>
 q_bad=<0|1> j_bad=<0|1> q_debt=<row.debt.q> j_debt=<row.debt.j> shortest=<min edge measure.length>
 longest=<max edge measure.length> epoch_now=<selection.repair_epoch> features=<n>
 f=<count>:<v0>:<v1>:<v2>:<demand.epoch>:<refusals[0]>:<refusals[1]>:<refusals[2]>,<next feature>,...
```

It is one physical line (the breaks above are for reading). Print the three refusal bytes as decimal
integers. Print `q_debt` and `j_debt` with 17 significant digits. The edge lengths are those that
`CollectQualityCensus` reads (`selection.edges.at(edge).measure.length` for `row.edges`).
After the cells, when more than 8 cells qualify, print
`NATIVE3D_M28_STUCK_MORE rank=<r> stage=<stage> round=<stats.rounds> not_printed=<count>`.

### E3. Who wins the reservation

In `RoundState`: two print-only members `size_t lost_own = 0; size_t lost_other = 0;`
(reset where the other per-round counters of `RoundState` are reset).

In `RoundAcquisition::ReserveBatch`, after the test `if (!reserved.valid) {...}`, under
`options.timings && repairing`, in a small private helper `CountReservationLosses()`:

- `held` = the sorted cell ids of `dependency_batch.candidates[i].cells` for all own candidates `i`
  with `reserved.chosen[i]`.
- For each own active candidate `i` with `!reserved.chosen[i]`: if one of its dependency cells is in
  `held`, `++lost_own`; else `++lost_other`.

The helper reads `dependency_batch` before `SlotAcquisition::UnpackBatch` moves the entries.
Its local vector is diagnostic scratch and is not a term of an admission. If an allocation fails,
catch it, set both counters to zero for the round, and continue (print-only code must not fail a run).

Print: extend the existing row `NATIVE3D_PRIVATE_WORK` (the row that already ends with `choices=`)
with ` lost_own=<n> lost_other=<n>` at its end.

## 5. Part F: the wide round in production

Apply again the three paths of the failed candidate `5dffa5ca5a`
(`Common/include/adaptation/CNativeEngine3D.hpp`: `ProductionEngineOptions()`;
`SU2_CFD/src/drivers/CSinglezoneDriver.cpp`: the two remesher constructions;
`UnitTests/Common/adaptation/CNativeDistributed3D_tests.cpp`: the wide repeat case at 2 and 4 ranks),
with no other change. One commit, separate from D and E.

## 6. Gates

D and E (one commit or two; "identical results"):
1. Source check for the whole item before the first edit (R4), in `R/night_20261115/N18/source_check.md`.
2. Build with no new warning.
3. G1, G2, G3 (a and b), G4, GQ: identical as section 2 of the work order says, `fixedstorage=4120`.
   The new row families `NATIVE3D_M28_*` and the new last fields of `NATIVE3D_PRIVATE_WORK` are
   report rows; the comparators treat them as they treat `choices=`.
4. No full run for D and E alone. T0 stays the run `round_8_5r_B/G5/full574`
   (native maxima 2236.870 / 1416.499 s). The short gates prove the identity; the first run of part F
   is the full run of this item.

F (the mesh changes):
5. G1 identical with the wide repeat case added; G2 and G4 identical; G3 rule (a) is a gate, rule (b)
   is a gate only if the run prints no `NATIVE3D_PRIVATE_WORK` line; the safeguards of
   `R/shared_engine_regression_gates.json` as in N16 part C.
6. G5 run 1: all standing gates of both events; the files are not compared with P6c / P7; the
   event-1 prefix identity against T0 as in N16 part C (your accepted comparator); then the isolated
   restart of event 2.
7. G5 run 2, same binary: the six files are byte-identical to those of run 1. These files are then
   the new reference (rule of the user for an item that changes the mesh).
8. Purpose rule, evaluated after run 2. For each event take the smaller native maximum of the two
   runs (the host can be busy in one run). Pass when both lines are true:
   - event 1 value + event 2 value <= 0.90 x (2236.870 + 1416.499) = 3288.032 s;
   - no event value is larger than its T0 value.
   The sum S of N16 (quality join wait + communication + clearance) is a report row, not a gate.
   (This rule replaces the S rule of N16 for this item only. The order of the user that installs
   this file confirms it.)

## 7. If a gate fails

- D or E: R5. If the cause is not a transcription error: revert that part, mark it FAILED, and do
  part F only if part D is in the tree. Without part D, do not do part F.
- F, gates 5 to 7: R5 (one diagnosis pass, read-only). If it is not a transcription error: revert
  part F only in one commit, keep D and E, mark "8.5r2 F FAILED". No second attempt, no other width.
- F, gate 8 only: revert part F only, keep D and E, mark "8.5r2 F FAILED its purpose rule". The P6c /
  P7 references stay.
- In each failure of F keep the complete rank outputs of the failed run. They hold the M28 lines.

## 8. Report

`R/night_20261115/N18/reports/STEP_8_5R2_<build>.md` and a CSV with the columns of
`N16/reports/STEP_8_5R_575.csv`, plus for each rank and event: `lost_own` and `lost_other` (sum over
the rounds), the count of quality ownership evaluations (`NATIVE3D_PHASE_OWNERSHIP ... quality=1
evaluate=1`) and of quality migrations, and the quality/scheduling seconds.
Add a table "stuck cells": every `NATIVE3D_M28_STUCK` line of the last census of each rank, and for
each of these cells the first census (stage and round) in which it was printed. If no such line
exists in a run, write "no stuck cell".
Update `Handbook/ADAPTATION_TIMINGS.md` if part F passes.

## 9. Not in this item

A repair for a stuck cell, a selection that avoids reservation conflicts, another batch width, the
lane 8.5ab. The reviewer designs these after the numbers of this item.
