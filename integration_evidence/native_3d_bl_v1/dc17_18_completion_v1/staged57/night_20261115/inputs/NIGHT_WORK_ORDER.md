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
