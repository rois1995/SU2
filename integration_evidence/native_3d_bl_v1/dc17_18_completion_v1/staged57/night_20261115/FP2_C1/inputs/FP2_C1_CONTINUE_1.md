# FP2-C part 1, continuation 1: the default proof and run S2 on build647 (the acceptance of part 1)

Author: reviewer, 2026-11-23 (checked against your evidence). For the main agent. The user sends it; it is a user
order. It changes no production code; the test-code steps that it names are test code (R3). It ends the hold of
`FAST_PATH_STAGE2_C1.md` ("the text") section 5 item 5. Copy this file with its SHA256 into
`R/night_20261115/FP2_C1/inputs/`.

## Why

Your run S on build647 passed (commit `4730bd59ec`): the native maxima are 606.213 / 387.779 s (FX2: 614.463 /
410.522 s), the final meshes have 467,139 / 464,565 cells (FX2: 550,027 / 575,733). The reviewer's decision: part 1
is the candidate as it is. Parts 2 and 3 (the surface collapse, the engine flag) come later as their own items, on
top of an accepted part 1. So part 1 now gets its default proof and its second run.

## 1. What to do (no production edit; `SU2_CFD` of build647 and its test driver, rebuilt in item 1)

1. Verify that `git diff --stat 4730bd59ec -- Common SU2_CFD` is empty and that `SU2_CFD` has the SHA256
   `binary_sha256` of `R/night_20261115/FP2_C1/build647/build_status.json` (`b824f37e...6b4c`). Verify that no owned
   job runs. Then put the test code back to the state of build647 (test code only; the test code of FP2-C part 2
   step 0 stays in commit `ebf5bfff8f` and comes back with part 2):
   `git restore --source=4730bd59ec --staged --worktree -- UnitTests`; commit only these paths
   (`git commit -m "FP2-C1 continuation 1: test code = 4730bd59ec (test only)" -- UnitTests`). Build only
   `UnitTests/test_driver` with `build_fp1.build(work, 649, solver=False)` as
   `FP2_C2_0/run_c2.py` did for build648, in `R/night_20261115/FP2_C1/build649/`. Its `test_binary_sha256` must
   equal that of build647 (`689cfdac...cfd3`), its `source_pins_sha256` that of build647 (`0a0a2062...`), and
   `SU2_CFD` stays `b824f37e...6b4c`. Then every pin of `FP2_C1/build647/source_pins.json` holds and all runners of
   this order take `R/night_20261115/FP2_C1/build647` as the build directory (build649 only proves the hashes). If
   the test-driver hash differs: stop this order, write it in the progress file, and wait. (If an order before this
   one made this step already: verify the three hashes only.)
2. The default proof on this binary. The build receipt, the fixtures and G1 of the text section 5 item 2 and the lab
   of item 3 are fulfilled by build647: write their receipts in the report and do not repeat them. These run now:
   a. G2 and GQ; G3, G4 and the safeguards (regression gates here), with `regression` of
      `R/night_20261115/FX2/run_scoped_fx2.py`, called under the same `patch.multiple(scope, HERE=FP2_C1, ...)` as
      `FP2_C1/run_c1_fixtures.py:64-66` (a small test-code wrapper in `FP2_C1/`), so that the outputs go to
      `FP2_C1/scoped/build647/` beside its `fixtures.json`. Its references are those of FX2: the build640 receipts
      for the reference paths and the scheduling calls, N14 for G2, the N22 capture for GQ, the G3 reference of N14,
      the G4 filters and the safeguard keys (`merit481`, `saved481`, `saved_current`);
   b. one G5 run OFF (both options NO): all standing gates; the six files identical to the N21 W reference;
      `R/night_20261115/FX2/compare_full.py` (`--mode off`) against `R/night_20261115/FX2/G5/build645/OFF/
      full645_run1` gives no exact difference (for this run the reference of `compare_full.py` is build645, not
      build640; they are equal in the exact rows);
   c. one run CORE (`R/night_20261115/FP1/run.cfg`, SHA256
      `1ec6c26d49d4ec7c62103d5bd096471b5422290cd06c44b2f1073b7489f2b4c1`): all standing gates; the six files
      identical to `R/night_20261115/FP1/on_reference.json`. (Part 1 changes no decision without the surface option;
      with CORE only, the row `NATIVE3D_LOCAL_CORE` gets the new print-only fields; this run proves that the core
      without the surface option did not change.)
   The runner of b and c is test code (R3): a copy in `FP2_C1/` of `FX2/run_fp2_b_g5.py` with the reference of OFF
   `FX2/G5/build645/OFF/full645_run1` (build 645) and the prerequisites of build647
   (`FP2_C1/scoped/build647/scoped.json` of item a, `FP2_C1/cases/build647/lab_acceptance.json`), and with a mode
   CORE as in `FP2_B/run_fp2_b_g5.py` (`DEFAULT_CONFIG('CORE')` = `FP1/run.cfg`, expected hashes `core_reference`;
   no `compare_full.py` for CORE). The OFF config is that of FX2 (`FX2/G5/build645/OFF/full645_run1/run.cfg`, SHA256
   `5c0fe041...2d56`: the two option lines are absent, so both options are NO). Outputs:
   `FP2_C1/G5/build647/OFF/` and `FP2_C1/G5/build647/CORE/`.
3. If a gate of item 2 fails: one diagnosis pass (R5, read-only), no correction. Commit the evidence with "FP2-C1:
   default proof FAILED", then revert part 1 in one commit:
   `git restore --source=7e6953e721 --staged --worktree -- Common SU2_CFD UnitTests` (all test code after FX2 stays
   in the git history: `671858086e`, `8e3f1ce081`, `ebf5bfff8f`; keep the evidence). This replaces, for the default
   proof only, the sentence "In this part no step reverts" of the text section 5. Rebuild both targets; verify that
   every pin of `R/night_20261115/FX2/build645/source_pins.json` holds on disk, and write the two binary hashes
   beside `binary_sha256` / `test_binary_sha256` of `FX2/build645/build_status.json` (`4f85c349...` /
   `abdd2603...`; expected equal; a difference is a report, not a new revert). Label "FP2-C1 reverted; production
   sources = build645". Stop and wait.
4. If item 2 passes: commit and push ("FP2-C1 continuation 1: default proven"). Then run S2: the config of run S
   (`R/night_20261115/FP2_C1/run.cfg`, SHA256 `07b91a94...5382`) and the runner `FP2_C1/run_fp2_b_g5.py` with a mode
   S2 added (test code, R3: the same controller, gates and isolated restart; work folder
   `R/night_20261115/FP2_C1/G5/build647/S2/`, case `full647_run1`; it verifies S first). Then:
   - your run S of build647 is the first of the two runs. S and S2 pass all standing gates and give the same six
     files (SHA256 computed from the files on disk). Write them with their SHA256 in
     `R/night_20261115/FP2_C1/on_surface_reference.json` (the same keys as `FP2_B/on_surface_reference.json`, with
     an `acceptance_summary.json` in `FP2_C1/G5/build647/`; `status` "PASSED" or "OPEN for the user" after the time
     rule): they are then the reference for both options YES. `FP2_B/on_surface_reference.json` stays unchanged as
     history;
   - `compare_full.py`, S against S2 (`--mode s`): a report, not a gate; write the count of exact differences and
     the first difference;
   - the time rule: the native maximum of each event of S and of S2 is not above 1.05 times that of FX2's run S
     (614.463 / 410.522 s, so 645.186 / 431.048 s); a miss of this rule alone is "OPEN for the user", not a failure;
   - if S2 fails a standing gate, or S and S2 do not give the same six files: one diagnosis pass, label "FP2-C1 ON:
     FAILED; default proven", keep all code, stop and wait.
   Labels after S2 (R8): "FP2-C1: accepted (S = S2)" or "FP2-C1: accepted, time rule OPEN for the user".
5. The close, in each case: the report `R/night_20261115/FP2_C1/reports/FP2_C1_647_continue_1.md` (the receipts of
   item 2; for S, S2 and FX2's run S and for each event: the native maximum, the cells, points and proof faces of the
   final mesh, and the table "where the time is" with the parts and definitions of
   `FP2_B/reports/FP2_B_640_continue_1.md` section "Where the time is": the rank with the native maximum, and a
   second column with the maximum over ranks; make it by extending `FP2_C1/report_s.py`, test code), the progress
   file, `HANDOFF_Codex.md` and `HowToProceed.md`; `Handbook/ADAPTATION_TIMINGS.md` only if S and S2 passed. Commit,
   push, verify that no owned job runs. Then go on with the next order in the queue; if there is none, stop and wait
   for the user.
Each 6-rank run is the only heavy work on the machine (R1). Expected machine time of this order: about 3 hours.

## 2. Not in this order

A production code change. Parts 2 and 3 of FP2-C. A repeat of run S.
