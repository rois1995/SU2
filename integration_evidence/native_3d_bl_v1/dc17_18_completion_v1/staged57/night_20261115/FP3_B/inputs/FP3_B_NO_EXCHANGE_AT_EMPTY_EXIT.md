# FP3-B: no ownership exchange at a construction-collapse exit whose pass was closed (mode S)

Author: reviewer, 2026-11-26 (reviewed by Codex). For the main agent. The user sends it; it is a user order.
Copy this file with its SHA256 into `R/night_20261115/FP3_B/inputs/`.
Code base: `3ebfcfd3a7` ("FP3-A: accepted; no engine construction collapse in mode S"), build675.
Short names: R, N, "mode S", OFF, CORE, "worker stage", "the pass", engine and core file roots: as in the
order FP3-A (your copy in `N/FP3_A/inputs/`), called "A" below. P3 = the order FP2-P3 and its record.

Scope. One production change in the ownership evaluation of the engine, active only where the
construction-collapse queue is closed by A (mode S after step C completed). It corrects a side effect of A.
Where the queue is not closed (OFF, CORE, wall heights, the lab control value `2`), expect the behaviour of
build675. The change can alter the files of mode S again: the acceptance is the two-run rule for S.

## 1. Why (measured on your runs of A)

With the pass closed no candidate is attempted before the first `length_collapse_exit`. The ownership
evaluation of that exit (`EngineControl.cpp:1382` -> `EngineOwnership::PhaseOwnership(context, false, ...)` ->
`ShouldMigrate`, `EngineOwnership.cpp:242-273`) uses `OwnershipExchangePays`
(`Common/include/adaptation/CNativeEngine3D.hpp:23-33`): remaining work = max(0, demand - cells) x measured /
max(1, attempts). With `attempts = 0` this is not a rate. Your logs:
- P3 (`N/FP2_P3/G5/build673/C7/trial2/full673_run1`, rank 0, event 1):
  `NATIVE3D_PHASE_OWNERSHIP quality=0 evaluate=1 migrate=0 ... predicted_gain=199330.9 ... attempts=26995`;
- A (`N/FP3_A/G5/build675/S2/trial1/full675_run1`, rank 0, event 1):
  `NATIVE3D_PHASE_OWNERSHIP quality=0 evaluate=1 migrate=1 ... predicted_gain=5294993240.68 ... attempts=0`,
  then `committed=1 moved_cells=202212`, and a second evaluation with `quality=1 ... migrate=1`.
A (S1, S2 and the core-probe run) moved 202,212 / 39,423 cells at the collapse exit / the quality entry in
event 1 and 147,955 / 61,838 in event 2. P3 made no exchange at a construction exit; it moved 210,371 cells at
the first quality entry of event 1 and made no exchange in event 2. Beside A in your report: the final-proof
`reverse_seconds` went from 30.7 / 8.8 s (P3 S2) to 50.0 / 34.8 s (A S2); the final-proof memo hits went from
49.9 % / 84.5 % to 42.5 % / 45.7 %; the quality seconds from 210 / 59.5 s to 281-301 / 85-96 s, on a loaded
host. The logs do not separate the effect of the first exchange from the other mesh changes of A.

| Assumption | State |
|---|---|
| The exchange at the empty exit comes from `attempts = 0` in the rate | MEASURED (the printed gain is the value of the formula with one attempt) and read in the source |
| The decisions at the quality entry without the first exchange | NOT KNOWN (P3: one exchange in event 1, none in event 2). Reported by step 2 (the ownership rows) |
| The final proof and the quality step become faster again | NOT KNOWN (the host was loaded). Reported by steps 2 and 4; no gate |
| The mode-S files change against A | expected (another partition during the split generations). Step 3 makes the new pair |
| Where the queue is not closed nothing changes | from the form of the change. Tested by step 1 |

## 2. The production change

In `EngineOwnership::ShouldMigrate`: compute the raw result of `OwnershipExchangePays` and its gain and
cost exactly as today. Set the EFFECTIVE `pays` to false when `!quality &&
context.census.selection->collapse_closed`. Use the effective `pays` in the existing eligibility expression;
keep `DiagnosticAlways`, the vote and the print. So in Automatic mode, with the closed queue and `quality`
false, the row prints `migrate=0` with the computed gain and cost. Everything else of the function stays:
the same collective calls in the same order, the same statistics. The evaluation
with `quality` true (the quality entry and its later calls) is not changed. `OwnershipExchangePays` is not
changed. No other production change; the diagnostic variable of A stays as it is (value `2` keeps the queue
open, so there the exchange rule works as in P3).
Hidden MPI case (G1 does not change): at least two ranks, Automatic ownership, timings on; the SAME
zero-attempt inputs in two runs, chosen so that the existing profitability computation returns true; require
`migrate=0` with the queue closed and `migrate=1` with the queue open.

## 3. The work, the gates and the labels

A new build number and receipt folders under `N/FP3_B/`. One frozen binary pair. One owned heavy job; the host
preflight of R1 before each launch. Adapters: copies of the adapters of A (test code, R3), with the S
reference of A (`N/FP3_A/accepted_reference.json`) as the historical comparator of S. The runner rules of A
hold (probe variable `0` in the acceptance runs, verified and receipted; the one-key exclusion for G3 / C492;
each S trial launched and sealed by itself). Bind the copied build, source, order and receipt pins to FP3-B
and the base `3ebfcfd3a7`; the production diff is only the change of section 2. The adapters of A ask for
receipts that this order does not make: seal step 1 from the checks of THIS order (no CORE receipt); S run 1
needs that seal, S run 2 needs the sealed S run 1; the launcher of step 4 needs the accepted S run 2 and no
probe-1 receipt; the final report needs only the checks and observations of this order. Use the S reference
of A for the historical report, the S pair of this order for the six-file identity, and the S entry of the
P3 record for the comparison of the value-`2` run.

Step 0. One R4 source check of this text.
Step 1, the paths with the open queue (closure default, probe `0`):
1. G1 identical to the reference; the hidden case.
2. G2 and GQ against the accepted P3 captures: equal.
3. The five shared checks as in A step 1 item 3.
4. G3 with the closure at its default and the probe `0`, against the accepted P3 G3 case: the standing
   physical gates, the common accepted-edit prefix and the observer-adjusted entry-work identity. G4: the P3
   G4 evaluator on the same pinned native 2D runs of item 3.
Label: "FP3-B: default paths unchanged", or "... FAILED", or "... OPEN". Not evaluated in this order (write
it): the CORE full workflow and the OFF full workflow.
Step 2, S run 1 with its isolated restart (after step 1 passed). Gates: all standing gates of both events;
the suppression gate of A step 2, unchanged; and the OWNERSHIP gate, on the same rank and worker-stage
packets as the suppression gate: in each completed packet exactly ONE `NATIVE3D_PHASE_OWNERSHIP` row with
`quality=0 evaluate=1`, after the initial `NATIVE3D_STAGE stage=length_collapse_exit` and before the first
split-stage row after it (when there is no split-stage row: before the next phase-entry row or the final
exit row); that row has `attempts=0` and `migrate=0`; and no row with `quality=0 committed=1` anywhere in
the packet. Report all `quality=1` rows. Complete contrary evidence is a result failure; missing, cut or
malformed evidence follows the measurement and watchdog rules of A. Report (no
gate), beside A (S1, S2) and the P3 pair: the ownership rows of each event (evaluations, `migrate`, moved
cells); the tables of A step 2; the final-proof `reverse_seconds` and `share_seconds`; the number of
`quality_exit` -> `construction_entry` returns of each event with their seconds; the host load at the start
and the end of each event.
Step 3, S run 2 (same pair, inputs, settings) with its restart: the gates of step 2, and the six files equal
in the two runs. Then they are the new S reference.
Step 4, a time observation (after step 3; no gate, no acceptance): after S run 2 and its isolated restart
are terminal, one run S with `SU2_NATIVE_FP3_PROBE=2` after the normal host preflight. Its six files against
the S entry of the P3 record (equal expected). Report its time stamps, the host load, the native maxima and
the phase maxima (construction, quality, final proof, preparation) beside the two S trials of this order.
Name the observation "the behaviour of P3 with the accepted-collapse probe active". The times are
observations under a recorded load that was not controlled; call them "same day" only when the time stamps
say so.

Acceptance (when steps 1 to 3 passed): publish `N/FP3_B/accepted_reference.json` (`workflows.S` in the form of
A) and append to `AGENTS.md` section 2a: "- Since FP3-B the S reference is the `workflows.S` entry of
`R/night_20261115/FP3_B/accepted_reference.json` (no ownership exchange at a construction-collapse exit whose
pass was closed). It replaces the S entry of FP3-A for ordinary S runs. The FP2-P3 record stays for OFF, CORE,
G2, GQ, G3 and `saved_current`; its S entry stays the reference of the lab control value `2`." Update the
progress file, `HANDOFF_Codex.md`, `HowToProceed.md`, `Handbook/ADAPTATION_TIMINGS.md`. Label: "FP3-B:
accepted; no exchange at the empty collapse exit". Report `N/FP3_B/reports/FP3_B_<build>.md` and `.json`.
Then STOP.

Failures: the paragraph "Failures" of A holds, with "the production change of FP3-B" for "the connected
production change of FP3-A": a result failure that stays reverts ONLY the change of section 2 of this order
(A stays accepted); label "FP3-B: FAILED; the exchange rule stays". The time observation of step 4 never
fails the item. The rules R1, R3, R5, R6, R7, R8, R9, R10, R12, R13 hold. Expected machine time: about 2
hours.

## 4. Not in this order

A change of `OwnershipExchangePays` or of the evaluation at the quality entry. A change of the quality step,
of the final proof, of the geometry memo, or of the returns to construction. A change of the core. A time
gate.
