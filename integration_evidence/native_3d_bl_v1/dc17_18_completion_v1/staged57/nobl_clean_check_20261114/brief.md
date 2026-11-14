# Brief: clean check of the no-BL path on build 494 (M6, 6 ranks)

R = `integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57`.

## 1. Task

Run the no-BL Euler case one time, with no profiler, on a quiet machine. Compare it with the
accepted run of build 293. Report. Stop.

- Do not change source. Do not build. Do not fix. Do not design.
- Do not change a gate, a budget, a limit or a config value.

## 2. Why

Measurement v2 (`Handbook/ADAPTATION_M6_MEASUREMENT_V2_20261114.md`, `R/measurement_20261114_v2`)
ran this case on build 494 with a profiler on a busy machine. Both adaptations reached the
5400 s limit and were rejected. Build 293 accepted both (3640.7 s and 2314.9 s).

The `euler293_preservation_during_bl*.json` checks compared file hashes only. So this path was
not executed between build 293 and build 494.

The user wants to refactor the engine with the no-BL path as the code base. We must know if
this path is slower now, or if the profiler and the host load caused the failure.

## 3. Reference numbers

Reference run: `R/m6_runs/euler_build293_bounded_reference_mach_only_real_cfd50k/run.log`.

| Quantity (rank 0, first adaptation) | Build 293 | Build 494, profiled, busy host |
|---|---|---|
| Collapse pass exit, elapsed | 194.72 s | 1423.51 s |
| Collapses accepted at that point | 480 | 480 |
| First split record, elapsed / splits | 241.14 s / 5047 | 1609.19 s / 3520 |
| Second split record | 274.10 s / 9949 | 1737.87 s / 8442 |
| Inner native time | 3640.7 s, accepted | 5344.5 s, rejected |
| Result | 149051 points / 736323 tets | not accepted |

## 4. The run

- Program: `integration_evidence/build-integrated-v3-currenthost/SU2_CFD/src/SU2_CFD`, SHA256
  `0311b5a2a4f871bd82f48cd7cef3eac49a932cd1d90012bbc7e3c0d887e74ee6`. Verify before the run.
- Same command, config and inputs as Run B of measurement v2
  (`R/measurement_20261114_v2/run_B/provenance.json`), with these differences only: no `perf`,
  no `gdb`, no attached observer that stops or samples a rank. A `/proc` poll every 10 s for
  memory and load is permitted.
- New case directory. Do not overwrite any saved file.
- nice 19, one owned heavy job, at most 6 heavy processes on the machine.
- Quiet host: before launch, the 1-minute load must be below 1.0 and no other process may use
  more than 50% of a core. If this is not true, do not start. Do not stop or change processes
  that you do not own. Tell the user which processes are busy and wait for the answer.
- Record the load every 10 s during the run.
- Let the run reach its normal end.

## 5. What to report

**T1. Table of section 3 with a third column for this run.** Add the result of the second
adaptation (time, accepted or not, points, tets).

**T2. Phases of the first adaptation**, from the timers and markers of the run, beside the
build-293 values: collapse, surface split, interior split, repartition, quality, alternation,
final finish. Give wall seconds, accepted edits, elections and mean election time.

**T3. Per-rank exit timers** beside build 293: upper-bound seconds and calls, election seconds
and calls, update seconds, geometry trials and rejections, `other`.

**T4. Host load** during each phase (minimum, median, maximum).

**T5. Verdict, one of three, with the numbers that support it:**
1. The path is as fast as build 293 (each phase within 15%) and both adaptations are accepted.
2. The path is slower on a quiet host. Name the phases and the counters that grew.
3. Not decided. Say what prevented the decision.

**T6. Only if the verdict is 2.** Read-only source comparison between the pins of build 293
(`R/build293_source_pins.json`) and build 494 (`R/build494_source_pins.json`) for the code of
the slow phases. List the commits and the changed functions that can explain the growth of the
counters in T3. Rank them. Do not test them. Do not fix them.

## 6. Rules

- Numbers come from this run and from the saved logs. No estimates given as measurements.
- Stop only if the run does not start. Then report.
- No recommendation about the refactoring.

## 7. Report back

- One file in `Handbook/`, 80 lines at most, with T1 to T6.
- Small evidence under R, with provenance (program, config and input hashes, command, host
  receipts).
- One line in `HANDOFF_Codex.md` and in `HowToProceed.md` that points to the report. If the
  verdict is 2, say in both files that the Euler 293 timing is historical and that the no-BL
  path is slower on build 494.
- Then stop.
