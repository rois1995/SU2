# Step 8.5r: larger quality rounds (work order 2, item 3, N16)

Written 2026-11-18 by the reviewer for the main agent. The user sends the order that
puts this file in the place of step 8.4a. The rules R1 to R13 of work order 2 apply.

Source of the code names: commit `4787358e7a` (end of phase C). Phase D moves code. Map
each name by its text (R4, "reading used"). Line numbers are help only.

Short names: Eng = `Common/src/adaptation/CNativeEngine3D.cpp`, EngH =
`Common/include/adaptation/CNativeEngine3D.hpp`, Dist =
`Common/src/adaptation/CNativeDistributed3D.cpp`, NE =
`Common/src/adaptation/native_engine3d/`, Drv = `SU2_CFD/src/drivers/CSinglezoneDriver.cpp`,
R = `integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57`,
F570 = `R/night_20261115/N4/section6_3_after_N11/phase_C/G5/full570`.

## 1. Why (facts; M = measured in F570, C = read in the source)

1. (M) Step 8.4a is withdrawn. M27 shows: a repeated exact query is 2 % of the query
   time of the quality stage. The safe fixes give 0.6 % of the event.
2. (M) Event 1, mean of the 6 ranks, 2,181 s: communication 667 s, join wait 539 s,
   geometry bound 390 s. Quality stage only: join wait 539 s, communication 296 s,
   clearance 86 s. Event 2, 1,448 s: 320 s, 201 s, 63 s.
3. (M) The quality stage of event 1 has 1,274 rounds. Each rank commits 3 to 7 edits in
   one round, in the first window and in the last windows. The repair queue of one rank
   holds about 26,000 features. So the count of rounds comes from the batch size 8, not
   from an empty queue.
4. (C) One round has about 340 collective calls. Most do not depend on the count of
   candidates. The join wait of a round is the difference of the busy times of the
   ranks in that round.
5. (C) The directory calls take 256 features (`MaximumDirectoryFeatures`). A curved
   repair candidate asks 4 support stars (`RoundAcquisition::SelectRepairSupport`). So
   the protocol of today permits 64 candidates for each rank in one round. Only the
   engine has the bound 8 (`MaximumCandidateBatch`, arrays and checks).
6. (C) `Directory::ClearanceBatch` compares each proposal with each selected peer, cell
   by cell, and computes `CollisionBounds` two times for each cell pair. With 48
   proposals this is about 1 million cell pairs in a round. With 384 proposals it is
   about 66 million. This loop must change first.

Idea: a repair round takes up to 64 candidates for each rank. All the rest of the round
is the round of today: same snapshot, same reservation, same contact test, same
publication. The count of quality rounds falls by a factor of 5 to 8.

Predictions (not gates): quality rounds of event 1 from 1,274 to 150 - 300; join wait
from 539 s to about 330 s; quality communication from 296 s to 100 - 200 s; clearance
from 86 s to 15 - 30 s. Event 1 from about 2,180 s to 1,700 - 1,850 s for each rank.

## 2. What does not change

- No gate, budget, tolerance or limit (R1). `MaximumDirectoryFeatures`,
  `MaximumDirectoryScan`, `MaximumCavityCells`, `MaximumReplacementCells` and
  `MaximumCandidateBatch` keep their values.
- The construction stage, the surface stage, the BL stages and the final proof. A round
  with `repairing == false` must be bit-identical to today.
- The rule for the repair slots (debt share), `Selection::RepairBatch`, `Coalesce`, the
  two reservations, `ClearanceBatch` as a test, `Publish`, the census.
- The value 64 is the protocol bound of fact 5. It is one value. Do not try other
  values (R2). If part C fails, section 6 says what to do.

## 3. The three parts (three commits, in this order)

### Part A (identical results): boxes in the peer test of `ClearanceBatch`

File Dist, function `Directory::ClearanceBatch`.

A1. Add in the anonymous namespace, near `CollisionBounds`:

```cpp
// Boxes of one received proposal: one for each new cell, and one around all of them.
struct ProposalBounds {
  std::vector<std::array<su2double, 6>> cells;
  std::array<su2double, 6> all{INFINITY, INFINITY, INFINITY, -INFINITY, -INFINITY, -INFINITY};
};

// True when the two boxes have a gap in one direction. Same test as the cell loop of today.
bool Separated(const std::array<su2double, 6>& a, const std::array<su2double, 6>& b) {
  for (unsigned k = 0; k < 3; ++k)
    if (a[k] > b[k + 3] || b[k] > a[k + 3]) return true;
  return false;
}

// Computes the cell boxes of a proposal one time. Throws as CollisionBounds throws.
ProposalBounds BoundsOf(const CollisionProposal& proposal);
```

`BoundsOf` calls `CollisionBounds` for the cells `0 .. proposal.count - 1` and widens
`all` with each box.

A2. In `ClearanceBatch`, in the `try` block that fills `proposals`, after the loop
`for(auto& owner:grouped)` and before `std::vector<unsigned long> hits`: build
`std::vector<ProposalBounds> bounds`, one entry for each element of `proposals`, same
index. The own-cell loop that follows uses `bounds[index].cells[k]` in place of
`CollisionBounds(cell)`.

A3. In the peer loop (`for(const auto* proposal:ordered)`), for each `peer` of
`selected`:
- first `if (Separated(mine.all, other.all)) continue;` where `mine` and `other` are the
  entries of `bounds` of `proposal` and of `peer` (index = pointer minus
  `proposals.data()`);
- then the cell loop of today, with `mine.cells[i]` and `other.cells[j]` in place of the
  two `CollisionBounds` calls. `++stats.collision_checks` and `ConformingDisjoint` stay
  where they are.

Why the result is identical. `all` is the union of the cell boxes. If `all` of A and
`all` of B have a gap in direction k, each cell box of A and each cell box of B have
this gap. The loop of today then skips each cell pair and changes no counter. Minimum
and maximum of doubles are exact. A cell with a coordinate that is not finite throws in
the own-cell loop of today (it calls `CollisionBounds` for each cell of each proposal);
with A2 it throws in the same `try` block, with the same message.

Storage. The boxes are 48 bytes for each cell of a received page. `query_scratch` admits
`4 * sizeof(CollisionPage)` for each page; the pages and `proposals` use two of these.
`CollisionPage` holds `std::array<Cell, MaximumReplacementCells>`, so one page is larger
than the boxes of its cells. Verify in the source check that
`sizeof(CollisionPage) >= MaximumReplacementCells * 48`.
Then no new admission is necessary and the modeled bytes do not change. If the check is
false: stop part A and write it in the report (R4).

A4. Own test (R3): one unit test of the peer selection with three proposals: two far
apart (both selected), two that overlap with no shared node (the later one is deferred),
two whose boxes touch in one coordinate (`a[k] == b[k + 3]`; the cell loop runs, as
today). Compare `stats.collision_checks` with the value that the loop of today gives.

### Part B (identical results): the engine can use a wider repair batch; it is off

B1. EngH, below `MaximumCandidateBatch`:

```cpp
// Largest repair batch of one rank that the directory bounds permit: a curved repair
// candidate asks four support stars and one directory call takes MaximumDirectoryFeatures.
constexpr size_t MaximumRepairBatch = 64;
// Capacity of the per-round receipt and slot tables.
constexpr size_t MaximumRoundBatch = MaximumRepairBatch > MaximumCandidateBatch ? MaximumRepairBatch
                                                                                : MaximumCandidateBatch;
```

In Eng (both headers are visible there):
`static_assert(4 * MaximumRepairBatch <= MaximumDirectoryFeatures, "...");`

B2. EngH, `struct EngineOptions`, after `candidate_batch`:

```cpp
size_t repair_batch = 0;  // Candidates of one rank in one repair round; 0 = candidate_batch.
```

B3. EngH, after `EngineOptions`:

```cpp
// Candidates that one rank can propose in one round. A repair round uses the repair batch when it is set.
inline size_t RoundBatch(const EngineOptions& options, bool repairing) {
  return repairing && options.repair_batch ? options.repair_batch : options.candidate_batch;
}
// Bound that the storage admissions of one round use. It is the value of today while the repair batch is off.
inline size_t RoundCapacity(const EngineOptions& options, bool repairing) {
  return repairing && options.repair_batch ? options.repair_batch : MaximumCandidateBatch;
}
// Rounds between two progress windows. A wider round does more work, so a window has fewer rounds.
inline size_t WindowRounds(const EngineOptions& options, bool repairing) {
  return std::max<size_t>(1, options.stagnation_window * options.candidate_batch / RoundBatch(options, repairing));
}
```

With `repair_batch == 0` the three functions give `candidate_batch`,
`MaximumCandidateBatch` and `stagnation_window`: the values of today.

B4. `EngineSetup::CheckInitialWallPolicies` (NE `EngineSetup.cpp`). No new collective
call (the timer calls must stay equal). Pack the two controls into the two reductions
that exist:

```cpp
// One reduction pair agrees both batch controls; the pair is packed without loss.
const uint64_t batches = (uint64_t(options.repair_batch) << 32) | uint64_t(options.candidate_batch);
const auto batch_lo = TimedAllreduce(batches, CPassiveComm::Op::MIN, world.comm);
const auto batch_hi = TimedAllreduce(batches, CPassiveComm::Op::MAX, world.comm);
if (batch_lo != batch_hi || !options.candidate_batch || options.candidate_batch > MaximumCandidateBatch ||
    options.repair_batch > MaximumRepairBatch)
  failure.Set(1, 0, "Native 3D candidate batch must be bounded and equal across workers.");
```

When `batch_lo == batch_hi` all ranks have the same pair, so the local test is the same
on all ranks, as the global test of today.

B5. `RoundState` (NE `EngineRound.hpp`) gets two plain members:

```cpp
size_t batch = 0;       // RoundBatch of this round; set when the round head lets the round proceed.
size_t token_base = 0;  // Tokens that all earlier rounds used: sum of (ranks x their batch).
```

In `Adapt`, directly after the three tests of `decision` that follow
`round_head.Decide(pick)`: `round_state.batch = RoundBatch(options, repairing);`.

B6. Use `round_state.batch` in the place of `options.candidate_batch` at these places,
and at no other place:
- `RoundAcquisition::AcquireBatch`: the start value of `repair_slots`;
- `RoundAcquisition::SetRepairSlots`: `std::ceil(options.candidate_batch * share)`;
- `RoundAcquisition::AdmitBatchStorage`: the three factors;
- `RoundAcquisition::FetchBatch`: `ordinary_cells`.

B7. The id token (Eng, the `ContextValue` lines of `slot_state.epoch` and
`slot_state.token`). New value of the token:

```cpp
transfer_memory::Add(round_state.token_base, transfer_memory::Mul(world.rank, round_state.batch), slot)
```

At the end of the round, at `batch_bytes=0;++round;`, first:

```cpp
round_state.token_base =
    transfer_memory::Add(round_state.token_base, transfer_memory::Mul(world.size, round_state.batch));
```

Why. Today `token = (round * size + rank) * candidate_batch + slot`. While each round
has the batch `candidate_batch`, `token_base = round * size * candidate_batch`, and the
new token is the same number. With two batch sizes the formula of today can give one
token two times (a late construction round after wide repair rounds); the running base
cannot. The slot `epoch` has no other reader (reviewer's search); remove it from
`SlotState` if your source check confirms this.
Verify in the source check: `slot < round_state.batch` at each use of the token (repair:
`slot < choices.size() <= repair_slots <= batch`; other rounds: `slot < slots <=
candidate_batch`; single path: `slot == 0`).

B8. Tables and bounds. Replace `MaximumCandidateBatch` by `MaximumRoundBatch` where it
is a table size or a count bound of one round:
- `RoundState::refused_receipts`, `RoundState::lane_slots` and their two `ContextValue`
  lines in `Adapt`; `RepairBatchRoutes::receipts`; the parameter type of
  `EngineQueues::ServiceFrontier` (`.hpp` and `.cpp`);
- the checks `key.slot >= ...` and `choices.size() > ...` (`CoalesceOrder`, `Coalesce`),
  `count > ...` and `receipts.size() > ...` (`ServiceFrontier`),
  `refused_receipt_count == ...` (`Refuse`, `PlanChoices`, `ReserveBatch`),
  `receipt_count == ...` (`FinishProposalRouting`).
Replace it by `RoundCapacity(options, repairing)` where it is a factor of a storage
admission: the `scratch` of `Coalesce` and the two `MaximumCandidateBatch + 1` factors
of the admission in `ServiceFrontier`.
`MaximumCandidateBatch` stays in B4 and as the default of `candidate_batch`.
Verify in the source check that no `sizeof` of a changed table enters an admission
formula. If one does: write it in the report and keep that formula at its value of
today (R4).

B9. The window. `options.stagnation_window` has two readers that count rounds:
- `RoundHead::CheckStagnationWindow`: `round % options.stagnation_window` becomes
  `round % WindowRounds(options, repairing)`;
- `EngineOwnership::PhaseOwnership`, the cool-down test: `stats.rounds -
  quality_ownership_round < options.stagnation_window` becomes `... <
  WindowRounds(options, quality)` (`quality` is the parameter of the function; it is
  true only for the calls of the quality stage).
The equality check of `stagnation_window` in the setup (`window_lo`, `window_hi`) stays.

B10. Nothing sets `repair_batch` in part B. All results are those of today.

### Part C (the mesh changes): production uses the repair batch

C1. EngH: `inline EngineOptions ProductionEngineOptions()`: default options with
`repair_batch = MaximumRepairBatch`. Comment: why 64 (fact 5).
C2. Drv: the two `std::make_unique<CNativeRemesher3D>(nativeReference3D,
SU2Native3D::EngineOptions{}, ...)` calls pass `SU2Native3D::ProductionEngineOptions()`.
No other caller changes. The unit tests build their own options, so G1 does not change.
C3. Own test (R3): in one existing repair test of
`UnitTests/Common/adaptation/CNativeDistributed3D_tests.cpp` that runs with a batch
parameter, add the case `repair_batch = MaximumRepairBatch` at 2 and 4 ranks: the
result is valid, the final census is clean as in the case of today, and two runs give
the same cells.

## 4. Gates

Parts A and B (identical results; R10 and R11 apply):
1. One source check for the whole item before the first edit (R4).
2. Build with no new warning.
3. On the commit of B (it holds A): G1, G2, G3 (rules a and b), G4, GQ against the
   references of work order 2 section 2. All old rows, modeled bytes and timer calls
   equal; `fixedstorage=4120`. If a gate differs: run it on the commit of A alone to
   find the part, then R5.
4. G5 on the commit of B: all standing gates, six files identical (work order 2,
   section 2). No restart run (the event-2 files are those of the reference). This run
   is the time reference T0 of part C.
   Report for part A (not a gate): quality / clearance seconds of T0 beside F570.

Part C (the mesh changes; rule of the user for a new reference):
5. G1 identical, with the new case of C3 passed.
6. G2 identical, all rows, modeled bytes and timer calls (the run ends in the
   construction stage). G4 identical.
7. G3: rule (a) is the gate. Rule (b) is a gate only if the G3 run prints no
   `NATIVE3D_PRIVATE_WORK` line; if it prints one, rule (b) is a report.
8. The executed gates of `R/shared_engine_regression_gates.json`: a run with no
   `NATIVE3D_PRIVATE_WORK` line must be identical to its reference; a run with such
   lines must pass its own physical and acceptance rules, and its differences are a
   report.
9. G5, run 1: all standing gates of both events (5400 s; 19073 MiB with the job peak
   equal to the sum of the process peaks; physical validator; transfer; continued CFD;
   95 % timer coverage; join agreement; owned proof faces SUM). The six files are not
   compared with P6c / P7. Extra identity check in this run: for each rank, each
   `NATIVE3D_` line of event 1 before the first `NATIVE3D_PRIVATE_WORK` line has the
   same integer fields as in T0. Then the isolated restart of event 2.
10. G5, run 2, same binary: the six files are byte-identical to run 1. Then these six
   files are the new reference (write their SHA256 in the report and in the progress
   file).
11. Purpose rule, for each event, with the timer rows of the six ranks:
   S = sum of the exclusive seconds of quality / join_wait + quality / communication +
   quality / clearance. Part C passes when S(run 1) <= 0.80 x S(T0) in both events.
   Check the host load before T0 and before run 1 (R1) and write both in the report.

## 5. Report

`R/night_20261115/N16/reports/STEP_8_5R_<build>.md` and a CSV. For T0, run 1 and run 2,
each event, each rank and the sum: root seconds; quality rounds; quality commits and
trials; `private_work_used`; busy seconds; join wait; quality communication, clearance,
census and scheduling seconds; `collision_builds`, `collision_checks`; `conflicts`,
`proposal_duplicates`, `batch_candidates`, `batch_reserved`; the largest count of
choices of one rank in one round; windows and ownership migrations of the quality
stage; process peak and modeled peak; points and cells of the two final meshes; the
final quality rows. Beside each: the prediction of section 1 where one exists. No
tuning after the numbers (R2). Update `Handbook/ADAPTATION_TIMINGS.md` in place.

## 6. If a gate fails

- Part A or B: R5. They have identical results, so a difference is a transcription
  error or an error of this text. Revert the part that differs; part C needs both.
- Part C, gates 5 to 10: R5 (one diagnosis pass, one correction of a transcription
  error). If it fails again: revert the commit of C only, keep A and B, mark N16
  "C FAILED" with the first number that differs, and go on with item 4.
- Part C, gate 11 only: revert the commit of C, keep A and B, mark N16 "C FAILED its
  purpose rule", keep the evidence, go on with item 4. The references stay P6c / P7.
- In no case change the value 64, the slot rule, a directory bound or a gate.

## 7. Not in this item

The local lane (design 5.12, part 8.5ab), the interface move, the balance rule, any
change of the wall-point moves. The reviewer uses the numbers of section 5 to decide
the size of 8.5ab.
