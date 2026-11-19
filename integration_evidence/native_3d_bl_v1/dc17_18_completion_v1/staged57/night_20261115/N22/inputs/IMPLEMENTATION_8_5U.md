# Step 8.5u (item N22): the ownership decision without the clock (part M), then the census without a vote after each chunk (part C)

Author: reviewer, 2026-11-19. For the main agent. Base: the accepted state after N21.
All rules of section 1 of `AGENTS.md` apply (R1 to R13). R = `integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57`.

"Reference in force" = the accepted G5 files and G5 times after N21: the files and times of part W of N21 if it passed; else the
files of section 2 of `AGENTS.md` with the times T1 of part P of N21; else those files with the T0 times of
`N4/section6_3_after_N11/round_8_5r_B/G5/full574`.

Part M comes first because part C needs it: part C makes the seeding census faster, and today the ownership decision reads the
seeding time. Part M changes a decision rule. Part C changes no result.

## 1. Why

### 1.1 Part M: the mesh depends on the clock today

1. `EngineOwnership::ShouldMigrate` (`native_engine3d/EngineOwnership.cpp:241-293`) decides if the ranks exchange cells. This
   decision changes the mesh.
2. Its rule (lines 258-279) uses wall-clock values: `wall` (258), `estimate_seconds` (263), `rebuild` =
   `initialization_seconds + seed_seconds` (265), `warming` from `scope_build_seconds` (270), `stats.ownership_seconds` (273),
   and the budget clause `2 * cost < maximum_seconds - wall` (279).
3. `initialization_seconds` and `seed_seconds` are recorded only with `options.timings` (`EngineAdapt.cpp:40`, `:66-67`, `:185`;
   `EngineCensus.cpp:347`). So the decision also differs between timings on and off. This is against R11.
4. The margins are small. T0 (`full574`), event 2, decision 2: gain 452.9 against 2 x cost 442.9 (ratio 1.02). Event 1,
   decision 3: 302.6 against 250.3.
5. The cost model is wrong by a factor of 10 to 25: a real exchange costs 3.9 to 11.1 s (the committed rows of T0); the rule
   predicts 106 to 221 s. An exchange copies the census rows; it does not measure the cells again (`EngineOwnership.cpp:361-362`).
6. No other production decision reads a clock, except the deadline checks of `maximum_seconds`.

### 1.2 Part C: the census waits after each chunk of 10 cells

1. `EngineCensus::Measure` (`native_engine3d/EngineCensus.cpp:271-330`) measures chunks of 10 cells
   (`min(16, MaximumCavityCells / 6)`). The count of chunks is the maximum over the ranks.
2. Each chunk has 6 collective calls: the time check (line 313), two votes in `ImportMeasurementBatch` (134, 141), the election
   in `CavityMetricSource::ImportCells` (`CNativeMetricTransport3D.hpp:273`), the vote in `Admit` (238) and the last vote (268).
   T0, preparation of event 2: 80,703 communication calls = 6 x 13,406 chunks + 267.
3. None of these calls moves cell data. They carry failure, admission and time flags only. The import is local.
4. Each chunk costs the time of the slowest rank in that chunk. Rank 0, preparation, exclusive seconds of `communication`:
   75.9 s (T0 event 1), 158.6 s (T0 event 2), 198.5 s (W event 2). The own work of one rank there is 31 to 79 s.
5. The same function measures the changed cells after each commit and the cells at each phase entry
   (`EngineCommit.cpp:671`, `:1311`, `EngineControl.cpp:616`).

## 2. Part M: the change

### 2.1 The rule

The new rule is the old rule with each clock value replaced by a count. Unit: one cell measurement.

```
D = demand_sum (line 249)            G = global_cells
S = SUM over ranks of stats.initial_cells      U = SUM over ranks of stats.updated_cells      A = attempts (line 261)
gain = (D - G)+ * (S + U) / max(1, A) * (imbalance - 1)
cost = G
migrate when   DiagnosticAlways   or   (imbalance > 1.25  &&  gain > 2 * cost)
```

Derivation (write it as the comment of the rule):
- `(S + U) / max(1, A)` = cell measurements for each attempted edit up to now. It replaces `wall / attempts`.
- `(D - G)+ * (S + U) / A` = W, the measurements that remain, summed over the ranks. `(x)+ = max(0, x)` as line 262.
- With imbalance `i`, the busiest rank has `W / P * i`. After a perfect exchange it has `W / P`. The saving is `W / P * (i - 1)`.
  (The old form `remaining * (1 - 1 / i)` is the same value, because its `remaining` was the time of the busiest rank.)
- The cost is one measurement for each cell, `G / P` for each rank. It replaces the rebuild allowance. `P` cancels.
- The factor 2 and the threshold 1.25 stay. No new constant.
- The budget clause of line 279 is removed. It needs a clock, and the existing deadline checks (`WithinTime`, lines 33, 87, 94,
  339, 343, 399, 443) stop a run at its budget with the old ownership. In the logs its smallest margin is about 3030 s.

### 2.2 The edits

File `EngineOwnership.cpp:241-293`, and one public header.

- New pure function in a public header that a unit test can include (proposal: beside `PhaseOwnership` in
  `Common/include/adaptation/CNativeEngine3D.hpp`):
  `bool OwnershipExchangePays(double imbalance, double demand, double cells, double measured, double attempts)`
  = `imbalance > 1.25 && max(0., demand - cells) * measured / max(1., attempts) * (imbalance - 1) > 2 * cells`.
  It also gives gain and cost to the caller (two output values or a small struct), for the print.
- `ShouldMigrate`: remove `wall`, `estimate_seconds`, `rebuild`, `warming`, `predicted_cost` in seconds and their three
  reductions (lines 258, 263-273). Add one reduction: `measured = TimedAllreduce(double(stats.initial_cells) +
  double(stats.updated_cells), SUM)`. `locally_eligible = DiagnosticAlways || OwnershipExchangePays(...)`. The six demand
  reductions, the `attempts` reduction and the `world.maximum` of line 281 stay.
- `stats.ownership_predicted_gain` and `stats.ownership_predicted_cost` get the new gain and cost (the unit is now cell
  measurements; they are not printed). `stats.ownership_seconds` (lines 289, 490) stays as a statistic. No decision reads it.
- If `geometry_authority` or `started` is then not used in the function: remove the unused reference (no warning).
- The print row (under `options.timings`):
  `NATIVE3D_PHASE_OWNERSHIP rank= quality= evaluate=1 migrate= predicted_imbalance= predicted_gain= predicted_cost=
  demand_sum= global_cells= measured= attempts= uncertainty_factor=2`. The fields `predicted_gain_seconds`,
  `predicted_cost_seconds`, `estimate_seconds`, `rebuild_allowance`, `warming_allowance` go away. Correct your report scripts (R3).

### 2.3 Forecast for the full run (a report, not a gate)

With the logged values the new rule gives these decisions. "Ratio" = gain / (2 x cost).

| Run | Decisions | Same outcome | Changed | Smallest ratio of a "yes" | Largest ratio of a "no" |
|---|---|---|---|---|---|
| T0 (default width) | 35 | 34 | event 1, decision 3: yes -> no (new ratio 0.67) | 2.29 | 0.67 |
| W of N20 (wide rounds) | 19 | 19 | none | 2.72 | 0.52 |

So: if the wide rounds are the reference in force, the reviewer expects the same decisions and the same six files. If the
default width is in force, the reviewer expects one exchange less in event 1 and a different mesh.

## 3. Part C: the changes

Idea: each rank measures all its chunks without a collective call. The votes become local checks. One collective time check
and one collective vote come at the end of `Measure`.

In a run without a failure, a retry or a time stop inside the census, each rank executes the same local statements in the same
order with the same inputs as today: the same chunks, imports, `Admit` values (`stats.maximum_work_bytes`,
`atlas_budget.caller_bytes`), measurements and counters. So meshes, files, decisions, work rows, byte rows and batch counters
are identical. Only the counts and seconds of collective calls change. (With part M in, no decision reads these seconds.)

In a run WITH a failure, a retry or a time stop inside the census, the result is valid and deterministic but the trace can
differ from today: a rank that fails stops its own loop, the others finish theirs, then all get the same elected failure; a
rank whose import is refused halves its own chunk only (today all ranks halve; the reference runs have `batch_retries=0`);
each rank reads the time budget before each of its chunks, and the join makes it common. A false return of `Measure` always
ends the event (all callers), so no later step uses the partly measured state.

Not changed: chunk size, order of the cells, `Vote`, `Admit`, the measurement, the memory rules, limits and budgets. No field
in `Session`, `CellState` or a row that travels. No timer operation. `fixedstorage=4120`. With a surface target (the BL path)
the old collective statements run.

### 3.1 C1: a scope in which votes are local (`EngineExchange`)

Files: `native_engine3d/EngineExchange.hpp`, `EngineExchange.cpp:27-34`, `EngineAdapt.cpp:112` (lifetime).

- New state beside `private_repair`, with the same lifetime pattern (an engaged optional with a `ContextValue`):
  `struct DeferredVotes { bool active = false; CLocalFailure first; };`
- `EngineExchange::ElectTrialFailure`: when `deferred.active` it returns the local result with the statement of line 32. When
  its argument is failed, has a severity below 4 (not one of the sticky atlas severities 4, 5, 6 of `Vote`) and `first` is not
  failed: copy the argument into `first`.
- New small RAII type (proposal `DeferredVoteScope`): the constructor asserts `!active`, clears `first`, sets `active`; the
  destructor clears `active`. It does NOT set `private_repair`.
- `Vote` and `Admit` do not change.

### 3.2 C2: an import without its election

File `Common/include/adaptation/CNativeMetricTransport3D.hpp:269-275`: `CavityMetricSource::ImportCells` gets a last parameter
`bool elect = true`. With `false` it skips the election statements (`CLocalFailure`, `world.elect`, the copy of the elected
reason and flag). `++stats.imports`, `ImportCellsFrom` and `AccountImport` stay. `EngineExchange::ImportSource`
(`EngineExchange.cpp:135-147`) passes `!deferred.active`. No other class needs the parameter.

### 3.3 C3: `EngineCensus::Measure` in three parts (R9: three small functions)

Prefix: lines 281-310, not changed (`Admit`, the surface-target vote and the `TimedAllreduce` of the count stay collective).

`deferred = !surface_target && !private_repair` (`private_repair` is false at each `Measure` call; the term is a guard).

Chunk loop, proposal `bool MeasureChunks(..., CLocalFailure& first, bool& time_stop)` (returns "complete"). It owns the
`DeferredVoteScope` when `deferred` is true, and copies `first` out before the scope ends:

```
for (slot = 0; slot < count;)                      // count = the reduced maximum, as today: empty chunks run too
  if (options.maximum_seconds)
    over = (elapsed >= options.maximum_seconds)    // deferred: the local value; else: world.maximum(...) as line 313
    if (over)  deferred ? { time_stop = true; leave the loop }  :  the statements of lines 314-317
  batch = as line 319
  if (!ImportMeasurementBatch(...) || !MeasureBatchCells(...))
    deferred: if the chunk's `failure` is failed and `first` is not, keep it (covers lines 148-151, where no vote ran);
    leave the loop, not complete
  slot += batch;  if (seed) ++stats.seed_batches
```

- With `deferred` false the function does what `Measure` does today, statement for statement (also its returns).
- For lines 148-151 (`batch == 1`, import refused) in the deferred case: `failure.Set(2, 0, imported.reason)` before the return.
- A rank runs the empty chunks up to `count` as today (they set `atlas_budget.caller_bytes`, `donor_batches`, `seed_batches`
  and the import timer calls).

Join, proposal `JoinMeasurement`, only when `deferred` is true, outside the scope (`active` is false). Each rank reaches it:

```
timed_out = options.maximum_seconds && world.maximum(time_stop)
if (!complete && !time_stop && !first.Failed())  first.Set(2, 0, "Native3D census stopped without a recorded failure.")
if (!context.exchange.Vote(context, first))  return false      // collective; a failure wins, sticky atlas failures elect here
if (timed_out)  the statements of lines 314-317 (stop = TimeBudget, the seed / update reason);  return false
return true
```

One `Measure` call then has 1 or 2 collective calls after the loop, in place of 6 for each chunk.

## 4. Fixtures (your test code, R3)

New test cases get a tag that the G1 selection does not run. Do not edit an existing test case.

- M1 the rule (serial, pure function): `OwnershipExchangePays` with hand values. Examples: imbalance 1.25 -> false for all
  inputs; (imbalance 2, demand 3e6, cells 1e6, measured 2e6, attempts 1e6) -> gain 4e6 > 2e6 -> true; the same with measured
  0.9e6 -> gain 1.8e6 -> false; attempts 0 uses 1; demand < cells -> gain 0 -> false.
- M2 no clock (MPI, 2 ranks, an existing distributed fixture with `PhaseOwnership::Automatic` if one reaches an evaluation):
  two runs, `options.timings` on and off, give the same cells and the same owner of each cell. If no fixture reaches an
  evaluation in the limit of R3: write it; the source check item (d) below is then the evidence.
- C1 identity (MPI): the G1 tables at 2 and 4 ranks are this check.
- C2 failure in the census (MPI, 2 ranks, each run with a timeout). Time: the existing G1 case
  `CNativeDistributed3D_tests.cpp:2711` (`maximum_seconds = 1e-12`, stops in the census) must pass as before. Memory: no
  existing case fails inside the census; add one small case (a memory ceiling that one rank exceeds in a census chunk, not in
  an earlier admission). Check: all ranks return the same reason and stop code, and the test ends.

## 5. Gates

### 5.1 Part M

1. Source check `R/night_20261115/N22/source_check.md` (R4) for both parts, before the first production edit. For part M verify:
   (a) `stats.initial_cells`, `stats.updated_cells` and `stats.attempted` are counted without a condition on `options.timings`
   and belong to the same statistics object and the same span as the `attempts` of line 261; (b) no other reader of the removed
   seconds exists in a decision; (c) the tests that assert an exchange use `DiagnosticAlways` or `Disabled`; (d) after the
   edit, `ShouldMigrate` reads no clock and no value recorded only under `options.timings`. If (a) is false: do not implement
   part M and part C; write it and stop the item.
2. Build without a new warning. M1 passes; M2 passes or has its R3 note.
3. G1: tables identical to the reference. G2: identical (it stops before the first decision), with the `calls` and seconds of
   `communication`, `collective_calls` and `tracked_calls` as a report if an evaluation is inside. G3, G4, safeguards: as for
   an item with identical results.
4. GQ and G5, case "same decisions". First compare the `migrate=` values of each evaluation with the reference run.
   If they are all equal in GQ: GQ is identical to its reference, with these rows as a report: the `calls` and seconds of
   `communication`, `collective_calls`, `collective_seconds`, `tracked_calls`, the `NATIVE3D_PHASE_OWNERSHIP` fields.
   Then one G5 run (`N4/section6_3_after_N11/round_8_5u_M/G5/`): all standing gates; if its `migrate=` values are equal to
   the reference and the six files are byte-identical to the reference in force: part M passed with identical results.
5. GQ and G5, case "a decision changed" (expected with the default width: event 1, decision 3). Then part M is an item that
   changes the mesh: all standing gates pass in two G5 runs with the same binary and the six files are identical between the
   two runs. These files are the new reference; the GQ run of this build is the new GQ reference. Write each changed decision
   (old and new gain, cost, outcome) in the report. A changed decision that section 2.3 does not forecast is not a failure,
   but write it first in the report.
6. Time is a report in part M. If the native maximum of an event is more than 5 % above its value in the reference in force:
   mark it "OPEN for the user" at the top of the report. Do not revert for that. Continue with part C.
7. Commit and push ("N22 M"). The G5 files and times after part M are the reference in force for part C.

### 5.2 Part C (only when part M passed)

1. Source check for part C (in the same file): (a) no collective call exists inside the chunk loop other than the 6 of section
   1.2 item 2, in all callers and phases without a surface target; (b) `private_repair` is false at each `Measure` call; (c) a
   false return of `Measure` always ends the event. If (a) or (c) is false: do not implement part C; write it; keep part M.
2. Build without a new warning. C2 passes.
3. G1: tables identical to the reference (no test pins a count of census collectives; if one does, change that pinned count
   only and write it in the report).
4. G2 and GQ: identical to the reference, with these fields as a report only: the `calls` and seconds of `communication` in
   each phase; `collective_calls`, `collective_seconds`, `election_calls`, `election_seconds`; `tracked_calls`,
   `tracked_collective_seconds`, `tracked_worker_calls`, `nested_tracked_worker`, `nested_tracked_collectives`,
   `tracked_collective_call_seconds`; all seconds. All other integer rows, the modeled bytes, the timer calls of the other
   operations (also `census` with its rule, and `import`), `seed_batches`, `donor_batches`, `batch_retries`, the `migrate=`
   values and `fixedstorage=4120` are equal. Write old and new communication calls of each phase.
5. G3 (a) and (b), G4, safeguards: as for an item with identical results.
6. One G5 run (`N4/section6_3_after_N11/round_8_5u_C/G5/`): all standing gates; six files byte-identical to the reference in
   force. Note for the 95 % timer coverage gate: this part removes named communication seconds, so the coverage goes down a
   little (W run 1, event 1, rank 4 had 0.9575). The gate does not change. If it fails: it is a failed result gate (R5); write
   the unattributed seconds of each rank.
7. Purpose gate of part C: the native maximum of each event is not more than its value in the reference in force, and the sum
   of the two is not more than 0.95 x the sum of the reference in force.
8. Commit and push ("N22 C"). Its times are the new time reference. Update `Handbook/ADAPTATION_TIMINGS.md`.

## 6. Failure rules

- Part M, a result gate: R5. If it fails again or the cause is in this text: revert part M in one commit ("N22 M FAILED"),
  keep the evidence, do not do part C.
- Part M, case of gate 5 and the two runs give different files: revert part M ("N22 M FAILED"), write the first `migrate=`
  value or the first file that differs, do not do part C.
- Part C, a result gate or a hang (an MPI test or run that does not end; keep the last log lines of each rank): R5, then
  revert only part C ("N22 C FAILED"), keep part M.
- Part C, only the purpose gate fails: write the table of section 7, revert only part C ("N22 C FAILED its purpose rule").

## 7. Report

`R/night_20261115/N22/reports/STEP_8_5U_<build>.md`:
- part M: the source check answers; M1, M2; the gates; the table of all `NATIVE3D_PHASE_OWNERSHIP` evaluations of the
  reference run and of the new run (event, quality, imbalance, old gain / cost, new gain / cost, `demand_sum`, `global_cells`,
  `measured`, `attempts`, old and new outcome); case 4 or case 5; the native maxima beside the reference.
- part C: the source check answers; C2; the gates; for the reference in force and for the G5 run, for each event: the native
  maximum, and for each phase (preparation, construction, quality, final_proof) the mean over the ranks of the phase seconds,
  of the `communication` calls and seconds and of the `census` inclusive seconds; the timer coverage of each rank.
- Update the progress file, `HANDOFF_Codex.md`, `HowToProceed.md`. Commit, push, verify that no owned job runs, stop and wait
  for the user.

## 8. Expected numbers of part C (a forecast, not a gate)

Communication calls of the preparation phase: 80,703 -> about 270 (event 2), 34,008 -> about 270 (event 1).
Preparation: about -75 s (event 1) and -130 to -190 s (event 2). Construction and quality: the census waits there too; the
gain there is not known.

## 9. Not in this item

A better cost model of the exchange (the real cost is about 10 s; this item keeps the cautious cost "one measurement for each
cell"). A change of the evaluation points of the decision. The BL path of the census (a surface target). A larger chunk.
Another order of the cells. The round joins of the quality stage (lanes 8.5ab). A change of `Vote`, `Admit` or the memory
admission.
