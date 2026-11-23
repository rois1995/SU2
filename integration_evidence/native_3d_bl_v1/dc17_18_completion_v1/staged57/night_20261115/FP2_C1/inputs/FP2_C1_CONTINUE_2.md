# FP2-C part 1, continuation 2: restore part 1 and finish its acceptance (the failure was a watchdog under host load)

Author: reviewer, 2026-11-23 (checked against your evidence). For the main agent. The user sends it; it is a user
order. Copy this file with its SHA256 into `R/night_20261115/FP2_C1/inputs/`.
Do not stop a job that runs (R12): let the rebuild of the reverted state end first. When it ends, write its two
binary hashes beside those of build645 in the report; do no other step of the close of the revert.

## Why

Your default proof of part 1 found no difference: G2, GQ, G3, G4 and the safeguards passed; the run OFF gave the six
N21 W files and no exact difference; the run CORE passed both events and its six files equal the FP1 reference. The
only failure was the isolated restart of CORE: exit 124 from its watchdog of 600 s, during the final metric
computation, while the host had a load of 23.39 from other programs of the user (your preflight recorded them). The
same restart needed 149 s on build640. So this was a time effect of the host, not a result of part 1. You reverted
part 1 as `FP2_C1_CONTINUE_1.md` item 3 told you: that text was wrong for this case (it had no rule for a watchdog).
This order replaces item 3 of that text and restores part 1.
The other programs of the user must run (user, 2026-11-23): the host stays loaded. The machine has 8 cores; your
ranks run at nice 19 and get what is left. Do not wait for a calm host.
Rule for this order and for the later items of the queue: a watchdog of a helper (the `timeout 600` of the isolated
restart, `p7p8_r3_20261115/run_disk_restart508.py:194`; the 14400 s application watchdog of the full controller;
the wall limit of a lab or test wrapper) is not a gate. If it ends a step while every gate that was evaluated
before it passed, the step is NOT a failed result gate (R5 does not apply) and it causes no revert: write it with
the host load and repeat that step one time with a watchdog three times as long (this replaces, for such a step,
the sentence of R1 that another program of the user "is not a reason to repeat a run"). If the repeat ends by the
watchdog again, the step is OPEN for the user: keep all code, stop and wait. The gates stay gates: 5400 s for each
event, 19073 MiB, the physical validator, the transfer, the coverage, the identity of files where an order asks it.

## 1. What to do

1. Restore part 1 (no new code): `git restore --source=4730bd59ec --staged --worktree -- Common SU2_CFD UnitTests`;
   verify that `git diff --stat 4730bd59ec -- Common SU2_CFD UnitTests` is empty; commit ("FP2-C1 restored: the
   default-proof failure was a host watchdog; production sources = build647"). Build both targets with
   `build_fp1.build(R/night_20261115/FP2_C1/restored647, <the next free build number>, solver=True)` (the helper of
   build647 and build649). Its `binary_sha256` and `test_binary_sha256` must equal those of
   `FP2_C1/build647/build_status.json` (`b824f37e...6b4c`, `689cfdac...cfd3`), and every pin of
   `FP2_C1/build647/source_pins.json` must hold on disk. All runners keep `FP2_C1/build647` as their build directory
   (the new receipt only proves the hashes). Then all evidence of build647 stays valid (fixtures, G1, lab, run S,
   and of continuation 1: G2, GQ, G3, G4, the safeguards, the run OFF, the full run CORE with its six files). If one
   of the two hashes differs: write both values, stop and wait for the user (do not repeat the proof runs).
2. The isolated restart of CORE, one time, from the sealed outputs of your run CORE
   (`FP2_C1/G5/build647/CORE/full647_run1`), in the folder of the run CORE, so that its receipts are complete there.
   First move the files of the failed restart into `FP2_C1/G5/build647/CORE/restart_attempt1_watchdog/`
   (`failed_attempt.json`, `observed_payload_identity.json`, `restart_host_preflight.json`, `restart_launch.json`,
   `restart_wait.json`, `restart_outer.log*`, `restart_post_only.log*` if present,
   `disk_restart_full647_run1_event2/`, `disk_restart_full647_run1_event2_controller_launch.json`) with a manifest
   of their SHA256 and old paths (as `RCFD1/resume_off_restart.py` renamed its `failed_attempt.json`; keep the
   evidence). The restart helper is a copy `CORE/run_restart647_1800.py` of `CORE/run_restart647.py` (test code, R3;
   the pinned helpers are not edited) with two changes only: its retargeted source also replaces
   `"timeout", "600"` by `"timeout", "1800"` (asserted exactly one time), and it launches and names itself where the
   original names `run_restart647.py`. Launch it with
   `workflow.run_stage(CORE, 'restart', [python, -B, run_restart647_1800.py, --source-case, CORE/full647_run1])`,
   then `verify_restart` (WORKSPACE = CORE). Start it at once (no wait for the host); write the first value of
   `/proc/loadavg` at the start and the elapsed seconds beside the 149 s of build640. The validator and the gates of
   the restart do not change. Then write `CORE/payload_identity.json` and `CORE/run_receipt.json` as `run_case` of
   `FP2_C1/run_default_g5.py` does (artifacts also: the new helper and the manifest). "Run CORE complete" means that
   `run_default_g5.verify_run(CORE, 'CORE', ...)` passes.
   - If it passes: commit and push ("FP2-C1 continuation 2: default proven").
   - If it ends by the watchdog again: label "FP2-C1: default proof OPEN (restart watchdog, host load)", keep all
     code, stop and wait for the user. No revert.
   - If its validator fails a check (a real difference): one diagnosis pass, label "FP2-C1: default proof FAILED",
     keep the code, stop and wait for the user. No revert without a new order.
3. Then run S2 and close: read `FP2_C1_CONTINUE_1.md` item 4 from "Then run S2" (its "default proven" commit is
   replaced by the commit of item 2 here) and item 5 (the same runner, config, time rule, labels and report; the
   report also holds the history of this failure and its cause). S2 keeps its own restart watchdog of 600 s; if
   that watchdog ends its restart, repeat that restart one time as in item 2 (1800 s). A miss of the time rule
   alone is "OPEN for the user", as that text says; write the load of the host beside the times of each run.
4. Then go on with the queue. The revert `6825979092` and the label "FP2-C1: default proof FAILED" (`935fa3ff9a`)
   are superseded by this order. When part 1 has a label "FP2-C1: accepted (S = S2)" or "FP2-C1: accepted, time
   rule OPEN for the user" from item 3, the start condition of FP2-C part 2 holds ("reverted" and "failed" in its
   text refer to the state before this order); S0 = that commit. The texts are
   `reviewer_handoff/design/FAST_PATH_STAGE2_C2.md` (SHA256
   `fdd678fed866cb0fb9171316ca1f8e534ea848357918fb1956d95bdbb2bf7cae`) and, after it,
   `reviewer_handoff/design/FAST_PATH_STAGE2_D1.md` (SHA256
   `3ed64a17664f3d144c7549a8e847fafa071649a1e16106c36dd7f02e4bd645a3`); read them there (do not edit that folder);
   copy each with its SHA256 into its inputs folder when it starts. In both items the watchdog rule of this order
   holds: a time rule or a wall limit of a helper is a report beside the host load, not a gate.

## 2. Not in this order

A new code change. A repeat of the runs OFF, CORE (full run), G2, GQ, G3, G4 or of the safeguards. A change of a
gate.
