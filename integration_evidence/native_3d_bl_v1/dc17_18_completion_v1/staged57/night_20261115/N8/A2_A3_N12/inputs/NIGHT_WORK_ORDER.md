# Work order for the long unattended work (night of 2026-11-15 and after)

Author: reviewer, for the main agent. The user sends it; it is a user order.
The user and the reviewer are NOT available during this work. You cannot ask a question.
This file says what to do in each case. It is complete in itself; the files that it names
give the details.

Files (read them at these paths when an item starts):

- H = `/tmp/claude-1000/-media-rausa-4TB-SU2-Versions-SU2-NativeIntegrated/f3d988e5-f14f-4a96-b6fb-963ec11730e7/scratchpad/M6_FIX_HANDOFF.md`
- D = `/tmp/claude-1000/-media-rausa-4TB-SU2-Versions-SU2-NativeIntegrated/f3d988e5-f14f-4a96-b6fb-963ec11730e7/scratchpad/ADAPTATION_ONE_ENGINE_DESIGN_V1.md`
- C = `/tmp/claude-1000/-media-rausa-4TB-SU2-Versions-SU2-NativeIntegrated/f3d988e5-f14f-4a96-b6fb-963ec11730e7/scratchpad/CUT_PLAN_8_2.md`
- the two inventories named in C.

Copy H, D, C and this file with their SHA256 into your evidence folder at the start, as
you did before.

## 1. Goal

When the user comes back, this is true:

1. The no-BL M6 two-event workflow passes all present gates on the newest accepted commit,
   and its six output files are byte-identical to P6c.
2. The prepared results-identical fixes are in (P7, P8, B7) or each has a recorded reason
   with numbers.
3. `Adapt` is cut into units with identical results as far as the gates passed (plan C).
4. The design has its source check (one list) and its measured numbers (one table).
5. One morning report says for each item: PASSED, FAILED with numbers, or NOT STARTED.

The work does not end when the night ends. Go on in the order of section 3 until the
list is done. Never idle and never wait for an answer.

## 2. Rules for unattended work

These rules are the latest user order. Where an older text (AGENTS.md, H Part 0) says
"stop and wait for the reviewer", read it as "stop THAT ITEM, record it, and go to the
next item of section 3 that does not depend on it". Add this order to AGENTS.md.

R1. No gate, budget, tolerance or limit changes. 5400 s for each event, 19073 MiB for the
    job, geometry 1e-4, q / J / size / height gates, requested heights and progression.
    One owned heavy job, nice 19, at most 6 heavy processes on the machine. Check the
    host load and the free memory before each long run.
R2. No redesign and no own fix of production behaviour. You implement what H, D and C
    say. A production change that is not written there is not allowed.
R3. Your own test code is yours to make work (H Part 0). Fixtures, scripts and report
    helpers that you wrote: find the cause, correct, run again, until they test what the
    item asks. Limit: 90 minutes for one fixture. Never change production code or a gate
    to make a test pass.
R4. One source check for a whole item, before its first production edit. Write all
    contradictions in one list in the report.
    - A contradiction on a print-only counter or a report row: write it, continue.
    - A contradiction on a result where the written text has one clear reading that the
      source supports: use that reading, write it in the list as "reading used".
    - A contradiction on a result with no clear reading: do not implement that part.
      If the rest of the item is independent of it, do the rest. If not, skip the item.
R5. A failed result gate (outputs not identical where they must be, accept / reject,
    validity, geometry, quality, memory, time):
    - Preserve the evidence. Write the first number that differs.
    - Use ONE diagnosis pass of at most 60 minutes, read-only plus the existing counters,
      to name the cause. If the cause is a transcription or wiring error against the
      written proposal (your code does not do what the text says), correct it one time
      and repeat the failed check one time.
    - If it fails again, or the cause is in the proposal itself: revert the item in one
      commit (keep the evidence and the reverted commit hash in the report), mark it
      FAILED, and go to the next item that does not depend on it.
R6. Time limits, so that one item cannot take the whole night: 90 minutes for one
    fixture (R3); 60 minutes for one diagnosis (R5); 3 hours for item N3. At a limit:
    write the state, go on.
R7. Keep a progress file `R/night_20261115/progress.md` (R as in H). Update it after each
    sub-step: item, sub-step, commit, gate numbers, next action. After each compaction of
    your context, read this work order and the progress file again before you continue.
R8. Commit after each passed sub-step. Push the checkpoints. Never commit a state that
    fails its gates as "accepted"; a preserved failure is labelled as a failure.
R9. Readable code (D 7.1a) for all new code. Written exceptions are listed in C.

## 3. The items, in order

| Item | Content | Depends on | Text |
|---|---|---|---|
| N1 | Finish the connected item P7 / P8 / M18 / M20 / M21: fixture, ordered checks (a), (b), safeguards, ONE full two-event run | - | H 2.0d, 2.0d-r, 2.0e, 2.0e-r, 2.0e-r2, 2.0e-r3 |
| N2 | Cache fix B7, only if its condition (item 0 of H 2.0c) holds with the M18 rows of N1 | N1 passed | H 2.0c |
| N3 | Source check of the complete design, read-only, one list. First 5.9, 5.10, 5A, 5B; the proof of 5.9 item 5a; rows M11, M15, M17, M19 of D section 9 | - | H section 3, part 4a |
| N4 | The cut of `Adapt` with identical results (design step 8.2), phases A to D | reference build from N1 / N2 (see below) | C |
| N5 | Measure all number rows of D section 9 on the newest accepted commit: M1 to M10, M12, M13, M14, M16 (M18, M20, M21 come from N1) | - (better after N4) | H section 3, part 4c; D section 9 |
| N6 | More cuts with identical results, same method and gates as C sections 1, 2 and 5, your own inventory first: `CNativeRemesher3D::Remesh` (about 875 lines), then `MeshOptimizer` in Eng, then the split of `Selection` into a census part and a queue part | N4 complete | C |
| N7 | Morning report, state files, final checks | always | section 5 |

Notes for the items:

- N1. If the full run fails a result gate and R5 does not solve it: revert the production
  part of the item. The reference build for N4 is then the accepted F3b state (commit
  `48f07ad1a8` plus the report-helper correction). N2 is NOT STARTED (no M18 rows).
- N2. Condition false, or a failed gate after R5: B7 is out; N4 uses the N1 build.
- N3. Read-only. It never blocks. Write the list to
  `Handbook/ADAPTATION_DESIGN_SOURCE_CHECK_20261115.md`. Then continue with N4.
- N4. C has its own rule for a sub-step whose identity fails (correct one time, else
  revert the sub-step and continue with the next independent one). Record the reference
  rows for G2, G3 and GQ with the reference build BEFORE the first edit. Run G5 three
  times only (after phase A, after phase C, at the end). If G5 fails after phase A or C:
  find the sub-step with GQ and G2 on the commits of that phase (bisection, each a short
  run), revert that sub-step, repeat G5 one time. If G5 still fails: keep the last commit
  whose G5 passed as the accepted state, mark the later phases FAILED, go to N5.
- N5. Each number is one row: value, how measured, evidence file. A number that you
  cannot measure: say why, do not replace it. The diagnostic code must not change
  production results; prove it with G2 (and GQ when it touches the quality path).
  The small fixture of M12 (flat channel) is permitted. Do the rows that need no new
  code first.
- N6. Only when N4 is complete with a passed G5. The same rules: no behaviour change,
  identity of all integer counters, one commit for each sub-step.
- NOT in this work order, also when all items pass: design steps 8.3 to 8.7 (they change
  the mesh result; the reviewer writes their implementation text after N3 and N5), a
  quota from the job memory limit, any work on A5 / B5, any BL or RANS capability claim.

## 4. What "accepted commit" means during this work

A commit is accepted when its item or phase passed its gates. The newest accepted commit
is the base for the next item. Each full run compares its six output files with P6c
(hashes in `Handbook/ADAPTATION_M6_MEASUREMENT_CHEAP_GROUP_20261115.md`).

## 5. End of the work (N7), and each time you must stop for a reason of R1

1. Verify that no owned job is running. Write the terminal handles.
2. Update `HANDOFF_Codex.md` and `HowToProceed.md` in place: build directory, binary
   pins, the newest accepted commit, evidence paths, scripts, open items.
3. Write `Handbook/ADAPTATION_NIGHT_REPORT_20261116.md`:
   - one table: item / sub-step, PASSED, FAILED or NOT STARTED, commit, the gate numbers;
   - for each FAILED: the first number that differs, the cause if known, the reverted
     commit, the evidence path;
   - the one list of N3; the table of N5; the lists of C ("found during the cut",
     written exceptions, ten longest functions);
   - event times, peak RSS sums and timer-tree rows of each full run, beside the F3b
     values (2683.4 s and 1977.2 s; 13.95 GB and 17.70 GB);
   - each place where you used a "reading" under R4, and each correction of your own
     test code under R3.
4. Commit and push.

## 6. Addition of 2026-11-16 (user decisions; this section wins over the text above)

State when this was written (07:40): N1 FAILED and reverted (`becb4927c9`), N2 NOT STARTED,
N3 PASSED, N4 FAILED at A1 and reverted (`8039fc1b59`), N5 in its one corrected full
repeat, N6 and N7 NOT STARTED. Do not stop or restart the running job. Finish N5 first.

New order of work: finish N5 -> 6.1 (N1 again) -> 6.2 (N2) -> 6.3 (N4 again) -> N6 -> N7.
All rules of section 2 stay. The rule of 6 heavy processes counts your helper scripts too.

### 6.1 N1 again: the user accepts P7 / P8 with a new reference for event 2

Cause of the first failure (your diagnosis, confirmed by the reviewer): the partition
weights of the next event read `prepared_pieces` of the geometry cache
(`CNativeAffineReference.cpp` `StructuralEstimate`, `CNativeImport3D.cpp` 1762-1806,
MAX over the ranks). P7 changes which facets stay in the cache. So event 2 starts with
other weights and gives another valid mesh. This is a property of the proposal, not an
error of your code. The reviewer's statement "identical results" was wrong for event 2.

User decision: P7 / P8 are accepted when event 1 is identical to P6c and event 2 passes
all standing gates and is repeatable. No physical gate, budget or limit changes.

The "P7 reference" for event 2 is the three event-2 files of your first N1 run
(`R/p7p8_r3_20261115/first_result_failure.json`):

- `adapted_mesh_adap_00002.su2`            63690652de1af70f890994fb5054523403ba1dc27200123a660e573827aebc36
- `adapted_mesh_adap_00002.su2.native_ref` 36c45abe03d1dc38e523583cc3103cfb9c83b17983b3679df0c424fab8ff088a
- `restart_flow_adap_00002.dat`            44766f7f95f14d60011fb27ef1bb864c44e9649f782f2b9a0139c6766c93bc94

Do this:

1. Base: the newest accepted commit after N5. Put back the 16 code / config / test paths
   that `becb4927c9` reverted, with their content of before that commit. Keep the N5
   observation code and the N5 fixtures. A conflict in a test file is your own test code
   (R3). A conflict in a production file: both texts stay, P7 / P8 as written in H and
   the N5 observer as it is; write each such place in the report. No other production
   change.
2. Build. Run the unit cases of the item, then the ordered checks (a) and (b) of H with
   the safeguards (exact first collapse, C492, 481, current target, 2D at 1 / 2 / 4
   ranks). They passed before on this code; they run again because the binary is new.
3. Run ONE full two-event workflow. Gates:
   - all standing gates of both events: 5400 s, 19073 MiB (P8: reported job peak equal to
     the sum of the process peaks, lower bounds not above it), physical validator,
     transfer, continued CFD, isolated restart where H asks it, 95 % timer coverage, join
     agreement, P7 check c1 (owned proof faces SUM equal to the surface size);
   - event 1: three files byte-identical to P6c;
   - event 2: three files byte-identical to the P7 reference above. Equal hashes prove
     that two runs give the same files.
4. If all gates pass but the event-2 files differ from the P7 reference: do not revert
   yet. Run the full workflow ONE more time with the same binary and inputs. If the two
   new runs have the same three event-2 files: N1 is PASSED, these files are the P7
   reference from now, and the report has all three hash sets with the note "differs
   from the first N1 run; the build differs by the N5 observation code". If the two new
   runs differ from each other: the result is not repeatable; revert the item, FAILED.
5. If event 1 differs from P6c, or another standing gate fails: R5 as written.
6. After N1 is PASSED: where H, C or this work order says "six files byte-identical to
   P6c", read "event 1 files identical to P6c, event 2 files identical to the P7
   reference". This holds for gate G5 of C and for N5 / N6.
7. The M18, M20 and M21 rows of the accepted run are qualified. Put them in the N5 table
   in place of the unqualified rows. Report the event times and the timer-tree rows
   beside F3b (2683.4 s and 1977.2 s) and beside the first N1 run (2220.3 s, 1451.0 s).

### 6.2 N2 (B7): the user accepts B7 with new references, by the same rule as 6.1

(Changed 2026-11-16, 07:50, second user decision. It replaces the "limited gate" text.)

1. Compute the condition (item 0 of H 2.0c) with the M18 rows of the accepted N1 run.
   Condition false: write the rank values and the total, N2 is "condition false", go on.
2. Condition true: implement B7 as written, build, run the checks of H 2.0c, then one
   full two-event run (run 1).
3. All standing gates of both events must pass (the list of 6.1 item 3). If one fails: R5.
4. Compare the six files of run 1 with the references of 6.1.
   - All six identical: N2 is PASSED. The references do not change.
   - One or more differ: this is expected, no diagnosis pass. B7 changes what stays in
     the cache, and `CNativeEngine3D.cpp` 2957 reads `prepared_pieces` inside event 1
     too, so event 1 can change as well. Run the full workflow ONE more time with the
     same binary and inputs (run 2). All standing gates must pass in run 2.
     - Run 2 has the same six files as run 1: N2 is PASSED. These six files are the
       "B7 reference" from now. It replaces the references of 6.1 in G5 of C and in N6.
     - Run 2 differs from run 1: the result is not repeatable. Revert B7 in one commit,
       FAILED, and go on with 6.3 on the N1 build.
5. Report in each case: the six hashes of each run, the cell and point counts of both
   events, the event times, the RSS sums, the timer-tree rows and the M18 rows, beside
   the accepted N1 run and beside F3b.
6. The memory rule stays as written in H 2.0c: the job peak of each run must be at most
   19073 MiB. A run above it fails a standing gate (R5; the cause is then in the
   proposal, so B7 is reverted).

### 6.3 N4 again, with the corrected gate G1

C has a new section 2.1 (G1 is an identity gate with its own reference). Read C again
completely and copy it with its SHA256 into your evidence folder.

1. Reference build = the build of the newest accepted commit (after 6.1 / 6.2).
2. Record with it, before the first edit: the G1 reference (C 2.1), and G2, G3 and GQ
   again (the production code changed since `4798ea3936`).
3. Then C 2.1 item 5 decides about A1, and the cut goes on as C says. G5 uses the
   newest references: those of 6.1, or the B7 reference when 6.2 made one.
4. The list "unit tests that fail on the accepted base" goes into the morning report.

### 6.5 Design steps 8.3 and 8.4 (first part), accepted by the user on 2026-11-16

The user accepted design revision 2 with steps 8.3, 8.4 and 8.4a. Copy these two files
with their SHA256 into your evidence folder and read them completely:

- `/tmp/claude-1000/-media-rausa-4TB-SU2-Versions-SU2-NativeIntegrated/f3d988e5-f14f-4a96-b6fb-963ec11730e7/scratchpad/IMPLEMENTATION_8_3_8_4.md` (I)
- `/tmp/claude-1000/-media-rausa-4TB-SU2-Versions-SU2-NativeIntegrated/f3d988e5-f14f-4a96-b6fb-963ec11730e7/scratchpad/ADAPTATION_ONE_ENGINE_DESIGN_V1.md` (D, revision 2.1)

This replaces the line of section 3 "NOT in this work order: design steps 8.3 to 8.7"
for the parts that I names. Steps 8.4a, 8.5, 8.6 and 8.7 stay out.

| Item | Content | Results | Text |
|---|---|---|---|
| N8 | The wall geometry law as production code with its census; the requirements record as a view; the table of the switch places; measurement M6b | identical | I part A |
| N9 | Counters of the wall stage (refusal classes, guidance causes, call sites) and one run of the G3 fixture | identical | I part B0 |
| N10 | No progress veto for a needed wall split | BL wall stage changes; no-BL identical | I part B1 |
| N11 | Inside a failing face the law chooses the edge | BL wall stage changes; no-BL identical | I part B2 |

Order with the cut (6.3):
- These items do not need the cut. They name symbols, not line numbers.
- If the cut has not made its first production edit when you read this: do N9, N10, N11
  and N8 first (in this order: N9 and N10 are the ones that help the BL goal most), then
  the cut. Record the cut references (G1, G2, G3, GQ) after them.
- If the cut is in work: finish the phase that is in work (A, B, C or D) with its
  gates, then do N9, N10, N11, N8, then record G1, G2, G3 and GQ again and continue the
  cut.
- A failed item is reverted by R5 and does not stop the items after it, except: N11
  needs N8 part A1 (the law); if N8 A1 is not in, do A1 before N11. N10 does not need N8.

### 6.4 Morning report (N7)

Keep the file `Handbook/ADAPTATION_NIGHT_REPORT_20261116.md` and update it in place. Add
one row for each item of this section. The first N1 run and the first A1 attempt stay in
the report as history with their numbers.

### 6.6 Correction of 2026-11-16, 14:50 (reviewer errors; this section wins over 6.5, over I and over C where they differ)

Facts, from your evidence (`R/night_20261115/N9/R5_diagnosis.json`, `REPORT.md`,
`B0_table.md`):

- The G3 fixture has two clock inputs. The observer before `PrepareSurface` runs for
  30 s and copies its work into the shared atlas budget
  (`UnitTests/SU2_CFD/adaptation/fixtures/native_actual_surface_target_observation.inc:18`),
  and the stage has 388.8 s. Three runs with identical decisions gave 473, 486 and 459
  splits. So the gate "all old integer rows of G3 equal" cannot hold. This gate text was
  an error of the reviewer. N9 did what its text asks: 459 common edits identical, entry
  work minus observer work 5,113 in both runs, counter sums agree. Your revert followed
  the written gate; it was correct to do it.
- N9 table: the guidance check rejected 1,180 of 1,437 trials (82 %) for choices of the
  geometry queue and 10 of 217 (5 %) for choices of the size queue. Cause `incomplete`
  1,187 times: `GeometryGuidanceExcess` stops at the limit `before`, that is, the new
  wall faces have more summed excess than the old ones. Cause `no_progress` 3 times.
- 865 of the 912 operator refusals have the label `other_geometry` and the class
  `other`. So their cause is still not known.
- 3,508 of 4,737 selected choices are `deferred` before a trial.

1. Gate G3, for all items and for C, from now:
   (a) Physical part, as before: all private cells positive with sensors, zero upper
       refinements, the modeled storage and the job peak inside their limits, at least
       336 splits, the first 336 edits identical to the P6c reference.
   (b) Identity part, for an item with "identical results": the accepted edits (edge and
       point of each edit) are identical to those of the G3 reference run, up to the
       smaller accepted count of the two runs; and `entry atlas_work` minus the observer
       work is equal in the two runs.
   (c) All other integer rows of G3 (entry `atlas_work`, rounds, split count, exit
       counters) are a report beside the reference. They are not a gate.
   The G3 reference run is the C492 run of section 6.1 (486 splits). G1, G2, GQ, G4 and
   G5 do not change. If you find a clock input in one of them, report it; do not change
   the gate.

2. N9 again, before the next production edit of another item.
   - Put back the three production paths of `bd709cb4c1` on the newest accepted commit.
   - One addition, print only: for each guarded trial that an OPERATOR refuses (origin
     not `Guidance`), count the distinct texts of `result.reason`. A fixed array of 16
     entries (the first 96 characters of the text, and a count) and one count `more`.
     Print at the stage exit: `NATIVE3D_WALL_REASON text="..." count=`. No decision reads
     it. The sum of the counts must be the count of operator refusals (912 in the table).
   - Gates: build; G3 by rule 1 (one run); G2. Report the B0 table again with the new
     rows. Then N9 is PASSED.

3. New order: N9 (item 2) -> N8 part A1 only (the law) -> N11 (B2) -> N10 (B1, with the
   new text of item 5) -> N8 parts A2 and A3 -> 6.3 -> N6 -> N7.
   - The G1 reference that you record now on the accepted base stays the G1 reference
     for these items.
   - If a production edit of B1 with the OLD text exists when you read this: do not
     judge it. Let a live job end, keep its numbers as "B1, old text, not judged", put
     the production paths back to the accepted base in one commit, and follow this order.
   Reason of the order: the 82 % against 5 % (facts above). Reading of the reviewer (I,
   N11 tests it): the geometry queue gives the score of a failing face to its edges by
   their length. So it splits the longest edge, also when the curvature is along another
   edge. The two new faces then keep the error, the summed excess is twice as large, and
   the guidance check rejects the split. For such a split the check is right. B1 with
   the old text would accept these splits. So B2 comes first, with the check in place.

4. N11 (B2): the text of I part B2 does not change. Its gates: I "Gates of B1 and B2"
   items 1, 2, 4 and 5 as written; item 3 is replaced by:
   - the G3 fixture two times; validity as in rule 1 (a); the two runs have the same
     edits up to the smaller of their accepted counts;
   - purpose: in BOTH runs, for the choices of the geometry queue, (rejections of the
     guidance check) / (guidance trials) is below 0.70. The reference is 0.82
     (1,180 / 1,437; two older runs differ by 0.003 for all queues together). If it is
     not below 0.70: the reading of item 3 is wrong. Keep the numbers, revert B2 in one
     commit (R5, cause in the proposal), do NOT do N10, and go on with N8 A2 / A3;
   - report beside the N9 table: all B0 rows, `law_weighted_faces`,
     `law_fallback_faces`, accepted splits for each queue, rounds, measured cells, exit
     `geometry_bad`, `size_bad`, `geometry_debt`, `size_debt`. The exit rows depend on
     the clock; they are a report.

5. N10 (B1), new text of item 1 of I part B1. `geometric_change` is not called when all
   of these are true: the `Adapt` call has a surface target; the trial is an edge split;
   it is not a coarsening and not a repair; and
   - the edge comes from the size queue, or
   - the edge comes from the geometry queue, the estimate of A1 is supported for this
     edge, and `EstimateSaysNeed` is true for it.
   In each other case the guidance check runs as today. Items 2 to 5 of I part B1 stay.
   Counters: `wall_splits_without_guidance` for each of the two queues, and
   `wall_splits_with_guidance`.
   Gates of N10: I "Gates of B1 and B2" items 1, 2, 4 and 5 as written; item 3 is:
   - the G3 fixture two times; validity as in rule 1 (a); the same edits up to the
     smaller count;
   - purpose: in BOTH runs the accepted count at the exit is at least 1.10 times the
     larger accepted count of the two N11 runs. If not: keep the numbers, revert B1 in
     one commit (B2 stays), go on;
   - report: the list of item 4, `guidance_seconds`, `upper_seconds`, and for the splits
     without the guidance check: how many of their new wall faces fail the exact
     measure in `Selection::Add`.
   The G3 runs of the last passed item are the G3 reference for the items after it.

6. The gate texts that this section replaces: I part A "Gates of part A" (G3 by rule
   1); I part B0 "Gate" (G3 by rule 1, G2); I "Gates of B1 and B2" item 3 (items 4 and 5
   above); C section 2, row G3 (rule 1). The old purpose rule "`geometry_bad + size_bad`
   below 7,984" is withdrawn: the exit counts depend on the clock, and a split of one
   failing face can give two smaller failing faces before the error is inside the
   tolerance.

7. Not a task now, for the report only: the 3,508 `deferred` choices. The reviewer
   writes a text for them after the N9 reason counts and the N11 numbers.

### 6.7 One more print-only measurement: M24, the lane census (2026-11-16, 15:00)

Purpose: design step 8.5 (not accepted, do not implement it). The reviewer needs one
number to write its text: which share of the quality work a rank could do alone.
Text: D section 5.11 and row M24 of D section 9 (D revision 2.2; copy it with its
SHA256).

Item N12. Do it in ONE build with N8 parts A2 and A3 (all three are print only), with
the same gates and the same full two-event run. No extra run.

1. Where: the private loop of a round, for each slot that reaches `evaluate_candidate`,
   in the quality stage and in the construction stage. Under `options.timings` only.
2. Class of the slot, from data that the slot has already:
   - `local` = each `dependencies.cells[i].owner` is this rank;
   - `wall` = one or more `imported.cells[i].physical` facets are bound.
   Four classes: local / shared x interior / wall.
3. For each class and stage: trials, approved patches, commits, busy seconds of the
   slot (the clock that the private loop has), optimizer work.
4. For the commits of each class: the count with `SameSkin(old, fresh)` true, the count
   with no parameter edit, the count with an empty coverage delta. Compute `SameSkin`
   for this only where the code does not compute it already; its result must not be
   read by a decision.
5. Print at the stage exit, one line for each rank, stage and class:
   `NATIVE3D_LANE_CENSUS rank= stage= class= trials= approved= commits= seconds= work= same_skin= no_parameter_edits= no_coverage=`.
   The sums over the classes must agree with the ladder counters that exist.
6. Gates: identical results. G1, G2 and GQ with all old integer rows equal, G3 by rule
   1 of 6.6, G4, and the full run of N8 with its six files equal to the references.
7. Report: one table for each event (6 ranks x 2 stages x 4 classes), and the share of
   the busy seconds and of the commits in the class local + interior.
