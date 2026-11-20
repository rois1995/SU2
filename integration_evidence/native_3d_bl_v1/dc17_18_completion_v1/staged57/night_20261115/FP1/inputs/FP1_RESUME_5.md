# FP1 resume 5: names for the unnamed timer time (part N of FX1), then the full runs again

Author: reviewer, 2026-11-20, 16:50. For the main agent. The user sends it; it is a user order.
Base: `eb9500b5d6` ("FP1 H6 ON failed event 2 timer coverage"). All rules of section 1 of `AGENTS.md` apply.
R = `integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57`. "The text" = `FAST_PATH_STAGE1.md`; resume 1, 3
and 4 = `FP1_RESUME.md`, `FP1_RESUME_3.md`, `FP1_RESUME_4.md` (your copies in `R/night_20261115/FP1/inputs/`). Copy this
file with its SHA256 into the same folder. NRR = `Common/src/adaptation/native_engine3d/NativeRemeshRun.inc`.
Evidence: `R/night_20261115/FP1/resume5/`.

## Why

Your ON1 run: both events complete on all ranks; native maxima 1,572.119 s and 383.244 s (OFF of build608: 1,818.319 and
1,039.725 s). The only failed number is the timer coverage of event 2: 93.69 % against 95 %. The cause is not in the
new code: event 2 became 2.7 times shorter, and the time without a name stayed (24.2 s on rank 0). Of these, 10.6 s lie
between the snapshots `length_collapse_exit` and `construction_entry` (the ownership exchange after the collapse pass)
and 7.0 s between `construction_entry` and `exit`. Section 2a of `AGENTS.md` says that the reviewer's text must give
names to the unnamed time before named time is removed; this is that text (it is part N of the item FX1, checked
against the source on 2026-11-19). The stop of FP1 ON was correct by section 12.5; this order opens FP1 again.

## 1. Part N: four timer scopes (print-only; no result changes)

Each scope is a `SU2NativeTiming3D::Scope` with an EXISTING operation, in the phase that is open at that place. No new
operation, no new phase, no new field: `fixedstorage=4120` stays (R10). A scope does nothing when timings are off. Do
not use `JoinWait`.

| # | Place | Edit | Row that gets the time |
|---|---|---|---|
| N1 | `native_engine3d/EngineControl.cpp:631`: `if (repairing && !context.ownership.PhaseOwnership(context, true, atlas_budget)) return false;` | `if (repairing) { bool owned = false; { SU2NativeTiming3D::Scope ownership_timer(SU2NativeTiming3D::Operation::Communication); owned = context.ownership.PhaseOwnership(context, true, atlas_budget); } if (!owned) return false; }` | `construction / communication` |
| N2 | `native_engine3d/EngineControl.cpp:1353`: the call `PhaseOwnership(context, false, atlas_budget)` in the condition before `BeginLengthGeneration` | the same block for this call (the scope is open for this call only), then `if (!owned \|\| !context.control.BeginLengthGeneration(context, false)) return FinishRound();` | `construction / communication` |
| N3 | NRR, in `Partition`: the two calls `PrepareWeights(...)` and `RepartitionMesh(...)` | one block with `Scope(Operation::Communication)` around the two calls; not around `source.Build`; `Phase(4)` stays between the two calls | `preparation / communication` |
| N4 | `native_engine3d/EngineAdapt.cpp:860-866`: the private trial (`candidate_trial.Run()` and `AdmitPrivateCoverage(...)`) | `Scope(Operation::Candidate)` as the first statement inside the existing `try {` | `quality / candidate` |

The statements inside the blocks, their order and their results do not change (the short-circuit of N1 and N2 stays).
The third call of `PhaseOwnership` (`EngineControl.cpp:1583`) has a named scope already; do not change it.

## 2. Gates of part N (an item with identical results)

1. Build without a new warning. `git diff eb9500b5d6` over the code tree: only these four places.
2. All fixtures of FP1 pass. G1 identical.
3. G2 and GQ by the rule of section 2a (the comparator of N22 part C): `communication` rows are a report; N2 and N3 add
   calls there. No other row of the compared snapshots may change. G3 (identity part). The safeguards of section 2.
4. ONE G5 run with the option OFF: all standing gates; six files identical to the reference in force; `fixedstorage=4120`.
   Report the coverage of each rank and event beside the OFF run of build608 (expected: higher). Its native maxima are
   the "OFF times of this run day" for section 3.
Commit and push ("FP1 N") after items 1 to 3; a second commit after item 4.

## 3. Then: the two G5 ON runs, as sections 12.3, 12.4 and 13 of the text say

Directly after the OFF run, with the same binary: two G5 runs with `ADAP_NATIVE_LOCAL_CORE= YES` (the config copy of
resume 1). Gates 12.3 items 1 to 4 (all standing gates, the two runs give the same six files, the time rule against the
OFF run of section 2 item 4 with a miss "OPEN for the user"; the checks against the OFF reference are a report). The
report rows of 12.4 and of resume 1 section 4.
If a gate of an ON run fails: ONE diagnosis pass, the evidence and the first failing number in the report, mark
"FP1 ON: FAILED", keep all code (the default is OFF and proven), stop and wait for the user. No repeat by your own
decision in this order.

## 4. Not in this order

Parts P and M of the item FX1 (they come with their own order). A change of a gate or of the 95 % value. A new timer
operation or phase. A change of the new code of FP1.
