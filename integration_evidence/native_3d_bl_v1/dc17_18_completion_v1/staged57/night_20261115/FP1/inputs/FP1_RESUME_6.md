# FP1 resume 6: three more timer scopes (part S), then the two ON runs again

Author: reviewer, 2026-11-20, 20:10. For the main agent. The user sends it; it is a user order.
Base: `cc96ab9e29` ("FP1 N ON failed repeat event 2 timer coverage"; event 2 of ON2 at 94.84 % coverage). Its code
tree is that of `d76fa0d5bc`.
All rules of section 1 of `AGENTS.md` apply. R = `integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57`.
"The text" = `FAST_PATH_STAGE1.md`; resume 1 and 5 = `FP1_RESUME.md`, `FP1_RESUME_5.md` (your copies in
`R/night_20261115/FP1/inputs/`). Copy this file with its SHA256 into the same folder.
Evidence: `R/night_20261115/FP1/resume6/`.

## Why

Your stop was correct. The result of resume 5: OFF passed (six files identical). ON1 passed all gates. ON2: event 1
passed and its three files are those of ON1; event 2 failed one number, the timer coverage on rank 0 (94.84 % against
95 %; ON1 had 95.11 %). The new code is not the cause. Event 2 takes 383 to 393 s now, and about 19 s of the old engine
have no name. Where they are (your stage rows and timer snapshots of ON1, event 2, rank 0; all in
`construction / unattributed`): 2.5 s before `length_collapse_exit`; 9.9 s between `length_collapse_exit` and
`construction_entry`, and 21 rounds hold them (the rounds that change the lane or the generation of the length work
take 10.4 s; the 3,026 normal split rounds have at most 1.5 s without a name); 4.4 s after the `construction_entry`
snapshot, in the rebuild of the queues at the entry of the quality phase. The four scopes of part N gave names to
5.5 s only; the reviewer's estimate for them was too high. Section 2a of `AGENTS.md` asks for the reviewer's text
before more named time goes; this is that text. This order opens FP1 ON again.

## 1. Part S: three timer scopes (print-only; no result changes)

Each scope is one statement `SU2NativeTiming3D::Scope <name>(SU2NativeTiming3D::Operation::Scheduling);` in the form of
N1 to N4. `Scheduling` is the operation of the same kind of work today (`PickRound`, `EnableCollapseFilter`: they
rebuild and read the selection queues). No new operation, no new phase, no new field: `fixedstorage=4120` stays (R10).
A scope does nothing when timings are off. Do not use `Census` or `JoinWait`.

| # | Place (lines of `d76fa0d5bc`) | Edit | Row that gets the time |
|---|---|---|---|
| S1 | `native_engine3d/EngineControl.cpp`, function `BeginLengthGeneration`: after the block of references (its last line is `auto& working = *context.ownership.working;`, `:480`), before `if (!context.exchange.Admit(` (`:482`) | the scope as the first statement after the references; it is open to the end of the function | `<phase of the round> / scheduling`: `construction` in the G2 and GQ prefixes and in the no-BL G5 runs |
| S2 | `native_engine3d/EngineControl.cpp`, function `CompletePhaseEntry`: the `try {` of `:599` (its first statement is `if (!refine_geometry) selection.ChangePhase(repairing, working, field_identity);`) | the scope as the first statement inside this `try {`; it ends with the block, before the `catch`, before `Vote` and before `Measure` | `construction / scheduling` (`quality / scheduling` when a quality round calls the function). It adds no call in a snapshot that G2 or GQ read: the first `construction_entry` snapshot is printed before `CompletePhaseEntry` |
| S3 | `native_engine3d/EngineCensus.cpp`, function `GenerationCensus`: after the block of references (its last line is `auto& directory = *context.exchange.directory;`, `:44`), before `const auto extra =` (`:45`) | the scope as the first statement after the references; it is open to the end of the function | `<phase of the round> / scheduling`: `construction` in the G2 and GQ prefixes and in the no-BL G5 runs |

- No statement, order or result changes. Each `return` stays. The collectives inside S1 and S3 keep their own
  `communication` rows (an inner scope keeps its own exclusive time).
- No scope may be open when a timer snapshot is printed. The reviewer's reading: S1 and S3 are function-local and their
  callers print after the return; S2 ends before `Vote`. If your source check finds a checkpoint print inside one of
  the three scopes: write it in the list and do not implement that scope (R4); the other scopes stay.
- Expected (a report, not a gate): 9 to 13 s get a name on each rank in event 2, more in event 1.
- Not in this part: an operation for the lane timer (`EngineAdapt.cpp:336`), a scope around `RoundHead::Decide`, a
  scope in the normal split rounds.

## 2. Gates of part S (an item with identical results)

1. Build without a new warning. `git diff` of the code tree against your base: only these three places.
2. All fixtures of FP1 pass. G1 identical.
3. G2 and GQ by the rule of section 2a (the comparator of N22 part C; `communication` rows are a report), with ONE more
   rule for this order: the `calls` of the row `construction / scheduling` are NOT compared with the old references
   (N14 for G2, the N22 capture for GQ). In the G2 and GQ prefixes part S adds calls to this row and to no other row;
   no new row appears. For this row:
   - in each `remesh` snapshot that the comparison reads (G2: `length_collapse_exit`; GQ: `length_collapse_exit` and
     the first `construction_entry`), the difference (this build minus the reference) is the same number on all six
     ranks; and at `length_collapse_exit` it is the same number in the G2 case and in the GQ case;
   - report the differences beside the expected values: + 1 at `length_collapse_exit` (S3, the call of the collapse
     pass; 8,136 -> 8,137 calls); at the first `construction_entry` (GQ only) + 1 for each call of `GenerationCensus`
     and + 1 for each call of `BeginLengthGeneration` before it (in the OFF prefix 51 + 23 = + 74; 115,606 -> 115,680
     calls). A number that differs from "expected" is a report, not a failure;
   - write the new values (case, snapshot, rank, calls) in
     `R/night_20261115/FP1/resume6/scheduling_calls_reference.json`. From now on this file is the reference for the
     calls of this row in G2 and GQ.
   Test code for this rule (R3): a comparator copy in `R/night_20261115/FP1/resume6/`; do not edit
   `N22/compare_prefix.py` or your prefix runner of FP1. The copy takes the calls of this one row out of the strict
   list in the way part C takes out the `communication` rows. Its self-check must show: a changed call count of
   another row (for example `import`) still fails, and a missing `construction / scheduling` row fails.
   All other rows: exact, as before. If another check of this order (G3, G4, a safeguard) compares the calls of a
   `scheduling` row of any phase with an old run: the same rule holds for it (report the difference; no failure).
   Write it in the report.
4. G3 (identity part). G4. The safeguards of section 2.
5. No G5 run with the option OFF in this order. Part S is "G5 OFF pending"; the G5 OFF run of the next item (FX1) is
   its proof for the six files. This is a decision of this order; it is not a missed gate (R8: the commit says
   "pending", not "accepted").
Commit and push ("FP1 S: gates 1 to 4 passed, G5 OFF pending (FX1)") after items 1 to 4.

## 3. Then: the two G5 ON runs, as sections 12.3, 12.4 and 13 of the text say

The OFF run of this order. Where the text (12.2 item 4, the first line and item 3 of 12.3, 12.4, 12.5) and resume 1
section 4 name "the OFF run" or "the same binary": in this order that run is the G5 OFF run of build615,
`R/night_20261115/FP1/resume5/G5/OFF/full615_run1` (native maxima 1,778.627 / 1,029.081 s). Its binary differs from the
binary of part S by the three scopes only; that is intended. The sentence "The ON runs start only after 1 to 4 passed"
is met by the gates 1 to 4 of section 2 of this order together with that run. Your G5 runner can bind the OFF record of
build615 for the ON pair (today it asks for the same build and pins: change this in a copy in your evidence folder, R3).

With the binary of part S: two G5 runs with `ADAP_NATIVE_LOCAL_CORE= YES` (the config copy of resume 1). Gates 12.3 items
1 to 4: all standing gates in both runs; the two runs give the same six files; the time rule against that OFF run,
with a miss "OPEN for the user"; the checks against the OFF reference are a report. The report rows of 12.4 and of resume 1 section 4. Report the coverage of each rank and event of both
runs beside ON1 and ON2 of build615, and the seconds of `construction / unattributed` and `construction / scheduling`
at `length_collapse_exit`, the first `construction_entry` and `final` on each rank.
Expected (a report): the three files of event 1 are those of the ON runs of build615.
If a gate of an ON run fails: ONE diagnosis pass, the evidence and the first failing number in the report, mark
"FP1 ON: FAILED", keep all code (the default is OFF and proven), stop and wait for the user. No repeat by your own
decision in this order.
If both runs pass: FP1 is closed with "ON accepted"; record the six ON files with their SHA256 as the reference for the
option ON in `R/night_20261115/FP1/on_reference.json`. Update the progress file, the report
`R/night_20261115/FP1/reports/FP1_<build>_resume6.md`, `HANDOFF_Codex.md`, `HowToProceed.md` and
`Handbook/ADAPTATION_TIMINGS.md`. Commit, push, verify that no owned job runs, stop and wait for the user.

## 4. Not in this order

Parts P and M of the item FX1. A change of a gate or of the 95 % value. A new timer operation or phase. A change of the
new code of FP1. A G5 run with the option OFF.
