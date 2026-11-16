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
