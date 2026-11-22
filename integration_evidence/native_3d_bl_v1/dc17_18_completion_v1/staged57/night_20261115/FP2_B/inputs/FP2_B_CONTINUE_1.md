# FP2-B continuation 1: the default proof and run S2 on build640 (the acceptance of FP2-B)

Author: reviewer, 2026-11-22 (checked against your evidence). For the main agent. The user sends it; it is a user
order. It changes no code. It ends the hold of `FP2_B_CORRECTION_3.md` section 3 item 5 and gives the path back to
`FAST_PATH_STAGE2_B.md` ("the text") section 6 item 3. Copy this file with its SHA256 into
`R/night_20261115/FP2_B/inputs/`.

## Why

Your run S1 on build640 passed: event 1 649.954 s (build638: 4,108.976 s; run CORE of build627: 1,512.056 s), event 2
446.547 s, no walk refusal, the first geometry repair 24.311 s (2,538.204 s before). The reviewer's decision after
this result: the faces above the tolerance that the core leaves (lab run N6P: 319, build638: 1,365; in the shifted
mesh 2 of them have a ring with a rank border and 317 do not) do not need one more step before the acceptance (the
first geometry repair of the engine now needs 24 s); they go to the next item. So the state of build640 (commit
`ecaed957db`) is the candidate of FP2-B.

## 1. What to do (no production edit; the binary of build640)

1. Verify that the sources under `Common`, `SU2_CFD`, `UnitTests` equal `ecaed957db` and that both binary hashes
   equal those of build640; verify that no owned job runs.
2. The default proof on this binary, as the text section 6 item 3 says: items 1 to 4 of FP2-A 11.2 with their
   references. Item 1 of FP2-A 11.2 is fulfilled by build640 (its build receipt without a new warning), the fixtures
   and G1 of correction 3 item 2, fixture (j) and the hand-over gate of correction 3 item 3 (they replace LAB-S and
   the rank check, as the text section 5 says): write their receipts in the report and do not repeat them. Items 2 to
   4 run now: G2 and GQ (`AGENTS.md` 2a, with `FP1/resume6/scheduling_calls_reference.json`), G3 (identity part), G4,
   the safeguards, the G5 run with both options NO (the N21 W six files), the run CORE (`FP1/run.cfg`, SHA256
   `1ec6c26d49d4ec7c62103d5bd096471b5422290cd06c44b2f1073b7489f2b4c1`, the six files of `FP1/on_reference.json`).
   The checks of the default proof that you made on build638 do not count (another binary); they stay as a report.
3. If a gate of the default proof fails: one diagnosis pass, no correction, mark "FP2-B: FAILED" (commit the
   evidence), then revert as `FP2_B_CORRECTION_2.md` section 5 item 7 says (`bea11c37f7`, with its verification;
   commit label "FP2-B reverted; production sources = build627"). The rebuild of that verification is permitted; its
   hashes must equal `binary_sha256` and `test_binary_sha256` of `FP2_B/build634/build_status.json`. Stop and wait.
4. If the default proof passes: commit and push ("FP2-B continuation 1: default proven"), then run S2 (the same config
   as S1). Then:
   - S1 (your run of build640) is the first of the two runs. S1 and S2 give the same six files (SHA256 computed from
     the files on disk). Write them with their SHA256 in `R/night_20261115/FP2_B/on_surface_reference.json`; they are
     then the reference for both options YES;
   - the time rule: the native maximum of event 1 of S1 and of S2 is not above 1.05 times that of the run CORE of THIS
     binary; a miss of this rule alone is "OPEN for the user", not a failure;
   - if S2 fails a standing gate, or S1 and S2 do not give the same six files: one diagnosis pass, mark "FP2-B ON:
     FAILED; default proven", keep all code (the option stays NO by default), stop and wait.
   Labels after S2 (R8): "FP2-B: accepted (S1 = S2)" or "FP2-B: accepted, time rule OPEN for the user".
5. The close, in each case (pass or failure): the report `R/night_20261115/FP2_B/reports/FP2_B_640_continue_1.md`,
   the progress file, `HANDOFF_Codex.md` and `HowToProceed.md`; `Handbook/ADAPTATION_TIMINGS.md` only if S1 and S2
   passed (the text section 7; with the note that `seconds_quality` of FP2-B includes the V rounds). In the report:
   the rows of the text section 6 item 4 for the run CORE beside S1 and S2, and for event 1 AND event 2 one table
   "where the time is" for S1, S2 and CORE: for each run, on the rank with the native maximum of the event, the
   seconds of each part (the core: surface steps, length work, quality with the V rounds, the state build; the
   engine: construction, the quality phase, each geometry repair, the final proof; the transfer), their sum, and the
   native seconds minus that sum (the rest); a part that does not run is written as 0. Give the maximum of each part
   over the ranks in a second column. Commit, push, verify that no owned job runs, stop and wait for the user.

## 2. Not in this order

A code change. A third pass of the core. A repeat of S1.
