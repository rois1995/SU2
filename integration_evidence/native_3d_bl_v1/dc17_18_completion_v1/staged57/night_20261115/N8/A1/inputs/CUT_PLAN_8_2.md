# Cut of `Adapt` with identical results (design step 8.2) - implementation plan

Author: reviewer, 2026-11-15. For the main agent. It belongs to
`ADAPTATION_ONE_ENGINE_DESIGN_V1.md` (sections 7.1, 7.1a, 7.2, 8) and to `M6_FIX_HANDOFF.md`.
All rules of AGENTS.md and of Part 0 of the handoff stay in force.

The facts come from two read-only inventories of the function (helpers, status C: verify):

- `/tmp/claude-1000/-media-rausa-4TB-SU2-Versions-SU2-NativeIntegrated/f3d988e5-f14f-4a96-b6fb-963ec11730e7/scratchpad/cut/CUT_INVENTORY_PART1.md`
  (function start to the round loop: 112 state variables, 37 lambdas, the setup blocks);
- `/tmp/claude-1000/-media-rausa-4TB-SU2-Versions-SU2-NativeIntegrated/f3d988e5-f14f-4a96-b6fb-963ec11730e7/scratchpad/cut/CUT_INVENTORY_PART2.md`
  (the round loop: 41 round blocks B01-B41, 22 slot blocks S01-S22, 30 blocks E01-E30 in
  `evaluate_candidate`, 36 lambdas).

Their line numbers are for the snapshot `scratchpad/cut/snap.cpp` (taken during the P7 / P8
work). Use the NAMES of lambdas, variables and blocks. Read both files completely before
the first edit. Where an inventory statement is wrong, it goes in your one list (Part 0).

## 1. Goal and what "identical" means

`Adapt` (Eng, about 4900 lines) becomes a short function that calls units. The mesh, the
decisions and the counters do not change.

Identical means, for each gate run: the same output bytes where the gate has output files,
and the same value of EVERY integer counter in every `NATIVE3D_STAGE`, `NATIVE3D_QUALITY`,
`NATIVE3D_DETAIL` and `NATIVE3D rank=` row, for each rank, in the same row order. Seconds,
RSS and other measured values are not compared. This includes `tracked_calls`,
`collective_calls`, `election_calls`, `maximum_work_bytes` and the timer-tree `calls`.

No behaviour change of any kind in this step: no merged code paths with different conflict
rules, no removed collective, no changed reserve / clear / swap of a container that
`bytes()` counts, no forwarded argument that is ignored today (`import_source` ignores
`requests`; it must keep ignoring it).

## 2. Gates

Reference: the last accepted build before the cut (after P7 / P8 and B7). Record the
reference rows one time with that build, with the same stop rules.

| Gate | What | When |
|---|---|---|
| G1 | Unit tests of the native 3D code that exist today, 1 / 2 / 4 ranks where they exist. IDENTITY gate against the G1 reference (see 2.1), not "zero failures" | each sub-step |
| G2 | Six-rank first collapse: work 90,809,869, counts 480/473/346/263/469/464, and ALL integer counters of all rows up to the stop | each sub-step |
| G3 | Serial BL repeat of C492, by rule 1 of work order section 6.6 (corrected 2026-11-16; the fixture has clock inputs): physical part; the accepted edits identical to the reference up to the smaller accepted count; entry work minus observer work equal; the other integer rows are a report | end of each phase, and after each sub-step that touches the surface / joint code (C8 to C11) |
| G4 | Native 2D tests, with and without BL, 1 / 2 / 4 ranks | end of each phase |
| GQ | NEW prefix gate: the six-rank M6 event 1 from the start to the third `NATIVE3D_QUALITY` window row, then stop as the collapse gate stops. ALL integer counters of all rows up to the stop equal to the reference | end of each phase, and after each sub-step that touches the repair path or the commit (C3, C4, C5, C6, C8, C10) |
| G5 | Full two-event run: six output files byte-identical to P6c, all standing gates (time, 19073 MiB, physical validator, 95 % timer coverage, join agreement) | after phase A, after phase C, at the end |

GQ exists because G2 covers the collapse path only and G3 is serial. GQ runs the splits,
the ownership transition, the private repair rounds and the batch commit on six ranks in
about 15 minutes. Write one small script that compares the integer counters of two logs
row by row and prints the first row and key that differ.

The identity runs need time budgets far from binding (two local time checks exist in the
private repair path). Do not run them on a host that is so loaded that 5400 s can bind.

### 2.1 Correction of 2026-11-16: G1 is an identity gate with its own reference

Cause of this correction. In the night of 2026-11-15 sub-step A1 (five local types moved
out of `Adapt`, bodies token-equal) was reverted, because G1 showed 13 failed assertions
and the 600 s limit of the unit wrapper ended. The first one is
`UnitTests/SU2_CFD/adaptation/BoundaryLayerMetric_tests.cpp:1220` (209.9357526597 against
656.1522579418). That test calls `ComputeMetric`, not `Adapt`. The test file and
`SU2_CFD/src/solvers/CSolver.cpp` are byte-identical to the accepted base. The other
twelve are in `UnitTests/SU2_CFD/adaptation/CNativeImport3D_tests.cpp` (lines 542 four
times, 1184, 1187, 1189, 1228, 1230, 1785, 2778 two times). No G1 reference was recorded
with the reference build, so nobody knew if these tests pass on the accepted base. This
was a hole in this plan, not an error of the cut.

The rule from now:

1. Before the first edit, run the complete G1 selection (the same 375 serial cases and the
   57 MPI cases at 2 and 4 ranks) with the REFERENCE build. Save for each test case: name,
   passed or failed, and for each failed assertion its file, line and the two printed
   values. This is the G1 reference. Save the run time of each cohort.
2. G1 passes for a sub-step when its table is equal to the G1 reference: the same test
   cases, the same passed / failed state, the same failed assertions with the same integer
   values. For a printed decimal value use 12 significant digits. A test case that passed
   on the reference and fails after the sub-step is a G1 failure. A test case that failed
   on the reference and passes after the sub-step is also a G1 failure (the cut must
   change nothing).
3. The time limit of the unit wrapper is your own helper (work order R3). Set it to 1.5
   times the reference run time of that cohort. A wrapper time-out is not a result gate;
   a test case that ends on the reference and does not end after the sub-step is one.
4. Do not repair, disable or change a test that fails on the reference. Write the list of
   these tests in the report with the title "unit tests that fail on the accepted base",
   each with file, line and the two values. The reviewer and the user decide about them
   later. They are not part of the cut.
5. If the G1 reference shows that all 13 assertions of A1 fail on the reference too, A1
   is not a failed sub-step: apply the preserved A1 diff again
   (`R/night_20261115/N4/A1_proposed.diff`), run G1 and G2, and continue with A2.
   If one of the 13 passes on the reference, A1 stays failed: write which one, with the
   two values, and stop N4 at that point (the cause is then not known).

## 3. Target structure

New folder `Common/src/adaptation/native_engine3d/`, one unit in one file pair. Names in
words. Each file starts with a comment: what the unit does, what it owns, what it must
never do.

| Unit | Owns | Content from `Adapt` (names from the inventories) |
|---|---|---|
| `EngineContext` | all long-lived state, grouped in one sub-struct for each unit; `RoundState`, `SlotState`, `RepairProposal` | the 112 variables of PART1 section 2; round and slot variables of PART2 section 2 |
| `EngineControl` | phases, budgets, deadlines, checkpoints | `enter_phase`, `begin_length_generation`, `surface_ready`, the checkpoint lambdas, `finish`, the round-head decisions B02 and B07-B29 |
| `EngineReports` | stage rows and diagnostics, no decisions | `observe_progress` (print part), `emit_quality_census`, `emit_fixed_metric_drift`, `record_rejection` |
| `EngineCensus` | measurement of cells into `Selection` | `measure`, `seed_next`, `generation_census`, `generation_debt`, `scope_census_complete`, `refresh_surface_census` |
| `EngineQueues` | choice of candidates | `coalesce`, `service_frontier`, `length_direction`, `pick`, `refuse`, `plan_choices`, acquisition B35 / B36 |
| `EngineExchange` | every collective and every directory call | `vote`, `elect_trial_failure`, `admit`, `bytes`, `real_bytes`, `import_source`, `retire_directory`, the trial switch (`trial_maximum`, `trial_reduce`, `trial_import_*`), reservation / routing / publication blocks |
| `EngineTrial` | validity and requirement checks of one candidate | `evaluate_candidate` shell, `height_check`, `metric_trial`, `geometry_trial`, `measure_private`, `validate_private`, the repair ladders E25-E29 |
| `EngineCommit` | apply an approved candidate | per-slot commit S14-S22, repair batch commit B40 |
| `EngineOwnership` | partition and migration | `phase_ownership` |
| `EngineSetup` | the setup blocks before the loop | PART1 section 4 (S10 to S19): option agreement, initialization, regions, first-row layer, seeding |
| `EngineLegacyWall` | the joint wall construction, moved AS IT IS | `place_patch`, strip search E21, apex search E22, growth loop E24 |

Three decisions of the reviewer:

1. `Selection` is not split in this step. It stays one class, used by Census and Queues.
   Reason: it is the largest coupling point and its split changes many call sites; it is
   step 8.2b, after the cut is proved.
2. `EngineLegacyWall` is a written exception to the 80-line rule (design 7.1a). Reason:
   design step 8.4 retires this code (`place_patch`, the strip search and the 12-growth
   loop). It has the densest lockstep protocol and the least gate coverage. Move it with
   the smallest possible change; do not split it into small functions; do not merge its
   three copies of the expand-cavity transaction.
3. Near-duplicates are moved, not merged (per-slot commit against batch commit; the three
   expand-cavity copies). Their conflict rules differ. The one exception: the sequence
   "donor import, then `FrozenField`" (8 copies) becomes one function if the 8 copies are
   textually equal up to names; if not, report the differences and keep them.

Two rules for state (from the identity risks of the inventories):

- `EngineContext` and its sub-structs cannot be copied or moved (deleted copy and move).
  `source`, `selection` and `directory` are built from the members that they reference
  (`working`, `stats`, `geometry`, `options.wall_heights`, `world`).
- The member order of `EngineSetup` and `EngineContext` is the present declaration order,
  so that construction and destruction keep their order (the geometry lease and the atlas
  geometry are released before the budget window is restored; the shape admission is
  restored last). `options` stays a mutated by-value copy inside the context.

## 4. Order of work

Before phase A, read-only, in your one list: do the constructors or destructors of
`SU2NativeTiming3D::Session`, `Directory` and `CavityMetricSource` make collectives? Is
`options.transaction_observer` the same on all ranks in each caller? Which `Selection`
functions write `stats`?

Phase A - state, no logic moves. The lambdas keep `[&]` and their text; only the names of
the variables change (`context.census.x`).
- A1. Local types to namespace scope: `RestoreAdmission`, `RestoreBudgetWindow`,
  `GenerationDebt`, `CoarseningCheckpoint`, `RepairProposal`.
- A2. `EngineSetup` for the RAII objects of the first lines (admission, timer session,
  budget window, atlas budget, private parameters and geometry, lease). Keep the early
  `return rejected` that does not call `finish`.
- A3. `EngineContext` with one sub-struct for each unit, built after `geometry` is
  resolved. One sub-struct in one commit (Control, Census, Queues, Exchange, Commit,
  Ownership), G1 + G2 after each.
- A4. `RoundState` and `SlotState`, declared at the same points as today. `CavityScope`
  and the timers stay separate locals in the same order.
- End of phase A: G3, G4, GQ, G5.

Phase B - the function-level lambdas become member functions. Order: leaves first.
- B1. Pure leaves: `selection_bound`, `geometry_trial`, `surface_merit`, `height_check`,
  `metric_trial`, `record_rejection`, `composed_sample`, `retire_directory`, the
  fixed-metric diagnostics, `length_direction`.
- B2. `EngineExchange` core: `elect_trial_failure`, `vote`, `admit`, `bytes`,
  `real_bytes`, `import_source`. `vote` must read the LIVE `private_repair` flag.
  `bytes()` lists exactly the same containers.
- B3. `EngineCensus` (`measure` split at the points of PART1 section 3.1).
- B4. `EngineQueues` (`coalesce`, `service_frontier`; keep the container and the input
  order of the receipt sort, it has ties).
- B5. `EngineReports`, then `finish` (same vote order, `observe_progress("exit")` at the
  same place; `finish` runs one time only).
- B6. `EngineOwnership` (`phase_ownership`, split at the points of PART1 section 3.1).
- B7. `EngineControl` phases and checkpoints.
- B8. `EngineSetup` blocks as functions that return a status; the caller does
  `return finish()` at the same points.
- End of phase B: G3, G4, GQ.

Phase C - the round loop.
- C1. Round head: B02 and B07-B29 as one decision function that returns
  Proceed / Restart / Finish / Break, one named function for each block. Keep each
  short-circuit expression as it is (it contains collectives). `Restart` must not advance
  `round` (the id ranges come from `round` and `slot`).
- C2. Acquisition: `refuse`, `plan_choices`, B35, B36.
- C3. Repair batch commit B40 (the join wait stays first, with its `JoinWait` scope).
- C4. Per-slot commit S14-S22, called INSIDE the slot body (the `CavityScope` and the slot
  `failure` must be alive). Do not remove dead branches here.
- C5. Slot acquisition S02 / S03 with a result Next / Proceed / Finish.
- C6. The trial switch as one small object (`private_repair`, `World&`, `source`) with
  `Maximum`, `Reduce`, `ImportCells`, ... and the same call targets. This object is the
  place where design step 8.5 later changes "collective" into "local in a pass".
- C7. Small lambdas of `evaluate_candidate` (PART2 step 9).
- C8. `measure_private`, `validate_private`.
- C9. `EngineLegacyWall`: move as it is (decision 2 of section 3).
- C10. Repair ladders E25-E29.
- C11. The shell of `evaluate_candidate`.
- End of phase C: G3, G4, GQ, G5.

Phase D - end.
- D1. Dead code, one item in one commit with G1, G2, G3, GQ: the non-streamed metric block
  with `try_repair` (`streamed` is always true), and the `repairing` branches of the
  per-slot commit. The collectives that still run in the collective path stay
  (`reserve_optimization(false)`, `trial_maximum(repair_budget>repair_work)`,
  `release_optimization`). If a gate differs, the code is not dead: put it back, report.
- D2. `Adapt` itself: 80 lines at most. Readability report as for F3 (ten longest
  functions; clang-format of the new files; the list of written exceptions).
- D3. G5 and the report.

## 5. Rules of work

- One sub-step = one commit = G1 + G2 (plus the gates of section 2 for that sub-step).
- An identity difference after a sub-step is a transcription error in moved code. Find it
  with the counter script (first row and key that differ) and by comparing with the
  original text. Correct it ONE time and repeat the gate. If it differs again: revert the
  sub-step, write the row, the key and the two values in the report, and continue with
  the next sub-step that does not depend on it.
- No fix of old behaviour during the cut, also when the old code looks wrong (examples in
  the inventories: an uncaught throw in the acquisition, the ignored `requests` argument,
  the receipt sort with ties). Write each one in a list "found during the cut".
- The timer-tree scopes keep their nesting and their operations.
- Readable code (design 7.1a) for all new files. The moved text inside a function keeps
  its statements; one statement on one line and clang-format apply to the new files.
- Report at the end of each phase: commits, gates with numbers, the list "found during the
  cut", the written exceptions, the ten longest functions.

## 6. Done when

1. G5 passes on the final commit: six files byte-identical to P6c.
2. `Adapt` is 80 lines at most. No function above 80 lines in the new files, except
   `EngineLegacyWall` and the functions of the list of written exceptions.
3. `Selection` and `MeshOptimizer` are untouched (their cut is step 8.2b).
4. The report has the two lists (found during the cut, exceptions).
