# Work order 2 (clean copy of the order in force)

Written 2026-11-17, 19:40, after the planned stop at the end of phase B of the cut.
Author: reviewer, for the main agent. The user sends it; it is a user order.

The planned stop is over. This order starts the work again (section 4). The rule
"never idle" is in force again.

This file replaces `NIGHT_WORK_ORDER.md` (sections 1 to 6.12) as the order in force.
It changes no gate, no rule, no limit and no result of a passed item. It removes the
layers of corrections and the items that are done or withdrawn. The old file and its
copies in your evidence folders stay as history. Where this file and an old text
differ, this file wins. If you find a rule of the old file that is still necessary and
is not here: use the old rule and write it in the report (R4, "reading used").

Files (copy each with its SHA256 into your evidence folder, read them completely):

- H = `/tmp/claude-1000/-media-rausa-4TB-SU2-Versions-SU2-NativeIntegrated/f3d988e5-f14f-4a96-b6fb-963ec11730e7/scratchpad/M6_FIX_HANDOFF.md`
- D = `/tmp/claude-1000/-media-rausa-4TB-SU2-Versions-SU2-NativeIntegrated/f3d988e5-f14f-4a96-b6fb-963ec11730e7/scratchpad/ADAPTATION_ONE_ENGINE_DESIGN_V1.md`
- C = `/tmp/claude-1000/-media-rausa-4TB-SU2-Versions-SU2-NativeIntegrated/f3d988e5-f14f-4a96-b6fb-963ec11730e7/scratchpad/CUT_PLAN_8_2.md`
- I = `/tmp/claude-1000/-media-rausa-4TB-SU2-Versions-SU2-NativeIntegrated/f3d988e5-f14f-4a96-b6fb-963ec11730e7/scratchpad/IMPLEMENTATION_8_3_8_4.md`
- I8 = `/tmp/claude-1000/-media-rausa-4TB-SU2-Versions-SU2-NativeIntegrated/f3d988e5-f14f-4a96-b6fb-963ec11730e7/scratchpad/IMPLEMENTATION_8_4A.md`
  (for item 3 of section 4 only; the reviewer writes it while you do the cut; do not
  look for it before you reach that item)

R = `integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57`.

## 1. Rules (the same rules as before, in one place)

The user and the reviewer are not available during the work. You cannot ask a question.
Where an older text says "stop and wait for the reviewer", read it as "stop THAT ITEM,
record it, and go to the next item that does not depend on it".

R1. No gate, budget, tolerance or limit changes. 5400 s for each event, 19073 MiB for the
    job, geometry 1e-4, q / J / size / height gates, requested heights and progression.
    One owned heavy job, nice 19, at most 6 heavy processes on the machine. Check the
    host load and the free memory before each long run.
    A heavy process is a solver rank, a build job, a test binary, or a script of ours
    that uses more than half of a core for more than 60 seconds. A browser or another
    program of the user is not a heavy process of the work. It does not fail a run and
    is not a reason to repeat a run. Record it in the receipt as an observation only.
    Your own helper scripts count (phase B: a renderer that was not awaited made 7).
R2. No redesign and no own fix of production behaviour. The reviewer supplies the ideas
    and the design. You implement what H, D, C, I and this file say. A production
    change that is not written there is not allowed. D is read-only for you.
R3. Your own test code is yours to make work. Fixtures, scripts and report helpers that
    you wrote: find the cause, correct, run again, until they test what the item asks.
    No limit on the count; limit of 90 minutes for one fixture. Never change production
    code, a gate or a tolerance to make a test pass. Write each cause and correction in
    the report.
    Do not change pinned fixture values; report changed counts or ranges.
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
R6. Time limits: 90 minutes for one fixture (R3); 60 minutes for one diagnosis (R5).
    At a limit: write the state, go on.
R7. Keep the progress file `R/night_20261115/progress.md`. Update it after each sub-step:
    item, sub-step, commit, gate numbers, next action. After each compaction of your
    context, and at the start of each new session, read this work order and the
    progress file again before you continue.
R8. Commit after each passed sub-step. Push the checkpoints. Never commit a state that
    fails its gates as "accepted"; a preserved failure is labelled as a failure.
R9. Readable code (D 7.1a) for all new code. Written exceptions are listed in C.
R10. An item with "identical results" adds no timer operation and no field to `Session`,
    `CellState` or a row that travels. These sizes go into the modeled bytes. Each
    `NATIVE3D_TIMER_SUMMARY` row prints `fixedstorage=4120`.
R11. Print-only code runs under `options.timings`. No decision reads its values.
R12. A new order never stops a running job. Let the check that runs end first.

## 2. Gates (C has the details; the lines below are the corrections that are in force)

- G1 is an identity gate with its own reference (C 2.1): the table of cases, pass /
  fail and failed assertions is identical. The totals of successful assertions are a
  report. Reference: the N14 set, `R/night_20261115/N14/section6_12/validation/adopted_reference/`
  (375 / 114 / 228 records for serial / 2 ranks / 4 ranks).
- G2 and GQ: all old integer rows, modeled bytes and timer calls of each row are equal
  to the reference. One exact rule for the operation `census`: for each rank and each
  timer boundary, the sum of `calls` over its rows is (reference value) + (count of
  `NATIVE3D_WALL_LAW` lines that this rank printed before that boundary) - (the same
  count in the reference run). For an item with identical results this difference is 0.
  References: `R/night_20261115/N14/section6_12/validation/cases/N14_G2_552` and
  `.../cases/N14_GQ_552`.
- G3 (the BL wall fixture; it has clock inputs, so its counts are not repeatable):
  (a) Physical part: all private cells positive with sensors, zero upper refinements,
      the modeled storage and the job peak inside their limits, at least 336 splits.
      (The old row "the first 336 edits identical to P6c" ended with N14.)
  (b) Identity part, for an item with "identical results": the accepted edits (edge and
      point of each edit) are identical to those of the G3 reference run, up to the
      smaller accepted count of the two runs; and `entry atlas_work` minus the observer
      work is equal in the two runs.
  (c) All other integer rows of G3 are a report beside the reference. They are not a gate.
  Reference run: the first G3 run of N14 (1,113 splits),
  `R/night_20261115/N14/section6_12/validation/cases/n14_g3_first_20261117T104157_3163858`.
- G4: as C says.
- G5 (the full two-event no-BL workflow): all standing gates of both events (5400 s;
  19073 MiB with the reported job peak equal to the sum of the process peaks; physical
  validator; transfer; continued CFD; isolated restart where C asks it; 95 % timer
  coverage; join agreement; owned proof faces SUM equal to the surface size). Event 1:
  three files identical to P6c. Event 2: three files identical to the P7 reference
  (your `R/night_20261115/section6_1/accepted_reference_payloads.json`). Where C says
  "six files byte-identical to P6c" (C section 2 row G5 and C section 6 item 1), read
  this rule. The event-2 hashes:
  - `adapted_mesh_adap_00002.su2`            63690652de1af70f890994fb5054523403ba1dc27200123a660e573827aebc36
  - `adapted_mesh_adap_00002.su2.native_ref` 36c45abe03d1dc38e523583cc3103cfb9c83b17983b3679df0c424fab8ff088a
  - `restart_flow_adap_00002.dat`            44766f7f95f14d60011fb27ef1bb864c44e9649f782f2b9a0139c6766c93bc94
- Safeguards for a change of shared code: the executed gates of
  `R/shared_engine_regression_gates.json` (exact first collapse, C492, 481, current
  target, 2D at 1 / 2 / 4 ranks).
- An item that changes the mesh is accepted only when its own text says so. The rule of
  the user for such an item: all standing gates pass, and two runs with the same binary
  give the same files. These files are then the new reference.

## 3. State (history; do not repeat a passed check)

| Item | Result | Commit |
|---|---|---|
| N1 (P7 / P8, new event-2 reference) | PASSED | `429c6c1a60` |
| N2 (B7) | FAILED (modeled storage above 12 GiB), reverted; D 2.2 withdraws B7 | `9831c787e7` |
| N3 (source check of the design) | DONE | `6941fcc41c` |
| N5 (measurements of D section 9) | PASSED | `3d28237f03` |
| N9 (wall counters) | PASSED | `9867041a2e` |
| N8 A1, A2, A3, N12 (law, record view, switch table, M6b, M24) | PASSED as one group | `f8c7b01085` |
| N11 (B2 alone) | FAILED its purpose rule, reverted | `0d9d627645` |
| N10 (B1, old text) | WITHDRAWN | - |
| N13 (M25) | PASSED | `8ab6da791e` |
| N14 (B2 + required split by the lift) | PASSED: 1,113 and 1,099 splits in the two G3 runs (limit 689), full run and restart passed | `0c2cb5c460` |
| N15 (M26) | PASSED in the same build and full run | `0c2cb5c460` |
| N4, the cut: phase A | PASSED with the full run and the restart | `5ec465ca1a` |
| N4, the cut: phase B (B1 to B8) | PASSED with G3, G4, GQ (build 558) | `97478b7aba` (B8 = `daf75b14e2`) |
| Planned stop after phase B | DONE | `f818510af9` |

## 4. The work from now, in order

1. The cut, phase C (C1 to C11) as C says, with the gates of each sub-step and, at the
   end of the phase, G3, G4, GQ and G5. Your whole-item source check
   (`R/night_20261115/N4/section6_3_after_N11/source_check.md`) stays valid; its
   reference readings from before N14 are replaced by section 2 of this file.
2. The cut, phase D (D1, D2, D3) as C says, with its G5.
3. N16 = design step 8.4a (the quality stage without an exact geometry query for each
   trial). The user accepted this step on 2026-11-16 and ordered it as the first step
   for the event time on 2026-11-17. Its complete text is the file I8 (section "Files").
   - When you reach this item: look for I8. If it exists, copy it with its SHA256, read
     it completely, and do it. The rules of section 1 apply (one source check first).
   - If I8 does not exist at that time: do item 4 first, and look again after it. If
     it still does not exist: do item 5 and stop. Do not write a text of your own for
     this step (R2).
4. N6: more cuts with identical results, same method and gates as C sections 1, 2 and 5,
   your own inventory first: `CNativeRemesher3D::Remesh`, then `MeshOptimizer`, then the
   split of `Selection` into a census part and a queue part. Only when the cut is
   complete with a passed G5. If N16 passed before, its reference files are the
   references of N6.
5. N7: the report. Keep `Handbook/ADAPTATION_NIGHT_REPORT_20261116.md` and update it in
   place: one row for each item of section 3 and of this section, with PASSED, FAILED or
   NOT STARTED, the commit and the gate numbers; for each FAILED the first number that
   differs, the cause, the reverted commit and the evidence path; each "reading used"
   (R4) and each correction of your own test code (R3); the list "unit tests that fail
   on the accepted base". Verify that no owned job runs. Update `HANDOFF_Codex.md` and
   `HowToProceed.md` in place. Commit and push. Then stop and wait for the user.

Rules of the cut that stay from the old file:
- G5 runs after phase C and at the end (D3). If G5 fails: find the sub-step with GQ and
  G2 on the commits of that phase (bisection, each a short run), revert that sub-step,
  repeat G5 one time. If G5 still fails: keep the last commit whose G5 passed as the
  accepted state, mark the later phases FAILED, go to the next item.
- A sub-step whose identity fails: C section 5 has the rule (correct one time, else
  revert the sub-step and continue with the next independent one).
- The report at the end of each phase, as C section 5 says (the phase B report is
  `Handbook/ADAPTATION_CUT_PHASE_B_20261117.md`).

## 5. Open items that have their own text

None. Each item of section 4 has its text in C, in I8 or in section 4.

## 6. Not in this work order

Design steps 8.5ab, 8.5c, 8.5d, 8.6 and 8.7; a quota from the job memory limit; any
work on A5 / B5; any BL or RANS capability claim. The reviewer writes the text for
8.5ab later (the M26 numbers of N15 are its input). Do not implement it before that
text is there and the user orders it.
