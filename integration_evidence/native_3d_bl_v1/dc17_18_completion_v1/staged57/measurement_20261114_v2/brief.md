# Brief: measure where the adaptation time goes (M6, 1 rank and 6 ranks)

This is a MEASUREMENT task. It comes before the design brief
`M6_STAGE3_WALL_FIRST_DESIGN_BRIEF.md`. That brief waits until the user has these numbers.

R = `integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57`.

## 0. Version 2: what changed after the first attempt

The first attempt (`R/measurement_20261114_v1`, report
`Handbook/ADAPTATION_M6_MEASUREMENT_20261114.md`) stopped Run A at 292 s and did not start Run B.
The cause was the stop rule of version 1. It was too wide: a decoder error in the OPTIONAL
call-graph step stopped all the work. The flat sampling was good. Keep that evidence.

Changes in this version:

1. A failure in the call-graph step does not stop a run. See section 6.
2. The call graph uses the processor's branch record (LBR), not DWARF. See section 4.
3. Run A reports the preparation time and the native engine time as two separate profiles (T0).
4. New table T7 asks for the callers of the hot functions that version 1 found.
5. Run A must run to its normal end. Then Run B runs.

Findings of version 1 that this version must confirm or reject (the reviewer computed them from
`run_A/snapshot_summary.json` and `run_A/samples_summary.json`; the split time is approximate):

- First 165 s of the process (mostly preparation, before the native engine): interval arithmetic
  in `SU2NativeGeometry` (`Multiply`, `Add`, `Adjacent`, `Blossom`) plus `__nexttowardl` is about
  70% of the samples. `__nexttowardl` alone is 19%.
- Next 127 s (native surface work, 12564 samples): `OptionA::InStrip` 28%, `BoundaryGuidance`
  lambda 23%, interval arithmetic about 33%, exact predicates 10%, tree containers 1.0%,
  memory 1.6%, cavity / topology below 0.1%.

## 1. Task

Measure. Report numbers. Stop.

- Do not change source. Do not build. Do not design. Do not fix.
- Do not change a gate, a budget, a limit or a config value.
- Do not give a recommendation about the refactoring. The user decides that with the reviewer.

## 2. Why

The user considers a large refactoring of the adaptation engine. The decision needs numbers that
do not exist yet:

1. About 40% of the 6-rank Euler 293 run is not covered by the existing timers.
2. A log read says that each rank waits 37 to 47% of the run. Nobody verified this with a profiler.
3. The mesh and the directory use `std::map` / `std::set` (134 uses in `CNativeEngine3D.cpp`;
   `using Owned = std::map<Id, OwnedCell>` at `Common/include/adaptation/CNativeDistributed3D.hpp:33`).
   Nobody measured their cost.
4. The wall upper-bound proof took 46% of serial run 492 by its own timer. The share of the rest
   is unknown.

## 3. Fixed facts (verified 2026-11-14)

- Program: `integration_evidence/build-integrated-v3-currenthost/SU2_CFD/src/SU2_CFD`,
  SHA256 `0311b5a2a4f871bd82f48cd7cef3eac49a932cd1d90012bbc7e3c0d887e74ee6` (pinned build 494).
  It has debug info, `-O2`, and is not stripped. Verify the hash before each run.
- `/usr/bin/perf` 6.8.12 is installed. `perf_event_paranoid` = 1. So user-space sampling works
  without root.
- The machine has 8 cores and 62 GiB RAM.
- The 4TB disk has only 17 GB free. The root/tmp disk is almost full. Keep all profiler data
  below 2 GB in total. Put it on the 4TB disk. Check free space before each run.
- The directory keeps the record of a point or a cell on rank `hash(id) % ranks`
  (`Directory::Rendezvous`, `Common/src/adaptation/CNativeDistributed3D.cpp:123-131`). This rank
  does not depend on the partition.

## 4. Runs

Run one at a time. Use nice 19. One owned heavy job. At most 6 heavy processes on the machine.
Check live job handles and host load first (AGENTS.md, resource discipline).

**Run A (serial, BL stage 3).** Repeat case
`R/cases/curved492_saved_production_surface_serial_20261113T163756_3724702` exactly: same input
(saved 485), same target, same config, same 388.8 s / 233220 work / 12 GiB. Write to a new case
directory. Do not overwrite `R/saved_surface492_checkpoint.json`, `R/surface492_adapter.json` or
`R/surface492_current_state`. Reference: 305 accepted splits, 389.57 s native.

**Run B (6 ranks, no BL).** Repeat `R/m6_inputs/euler_adapted293_event1_v1/run.cfg` exactly
(`ADAP_HAUSD= 1E-4`, `ADAP_NATIVE_MAX_TIME= 5400`, `ADAP_NATIVE_RANKS= 6`). Write to a new case
directory. Reference (older build): accepted, 736323 tets, exit at 3640.7 s. The program is now
build 494. Report each difference from the reference.

**Sampling.** Use `perf record` on each rank, one data file for each rank, at a low rate
(for example `-F 99`), with no call graph for the full run. This keeps the data small.
The program has no frame pointers, so plain `-g` gives wrong call chains.

**Call graph (optional step).** The processor is an Intel i7-9700. It has a branch record (LBR).
For the callers in T7, attach a second `perf record -e cycles:u --call-graph lbr -F 99` to one
rank for 60 s at most, one time in the preparation and one time in each phase in question.
Decode with `perf report --no-inline --no-children` or `perf script --no-inline`. The option
`--no-inline` is necessary: the `addr2line` error of version 1 came from the inline decoder.
Do not use `--call-graph dwarf`. If LBR is not available or the decode fails, write the error in
the report, leave T7 "not measured", and continue the run.

**Overhead check.** Compare native time and accepted edits with the reference. If the profiled
run differs by more than 5%, say so. Do not correct for it.

## 5. What to report

**T0. Run A in two parts.** Record the clock time when the native engine starts and when it ends.
Give T1 one time for the preparation (process start to engine start) and one time for the native
engine. Give the wall seconds of each part.

**T1. Share of samples by group, for each run and each rank.** Give the top 20 symbols too.

| Group | Examples of symbols |
|---|---|
| Wall upper-bound proof | `EntityFaceError`, Bernstein / subdivision boxes, `ErrorMagnitude`, `ClosestCurved` |
| Exact predicates | `CNativePredicates3D`, `SurfaceNormalSign`, orientation tests |
| Target / BL guidance | `BoundaryGuidance`, `CBoundaryLayerMetric`, metric evaluation |
| Tree containers | `std::_Rb_tree*`, map/set find, insert, erase, iterate |
| Memory | `malloc`, `free`, `operator new`, `memcpy`, `memmove` |
| MPI library | all `MPI_*`, `opal_*`, `ompi_*`, progress polling (this is waiting) |
| Pack / unpack | `Fields`, serialization, `World::exchange` user-side code |
| Cavity / topology | split, collapse, swap, star and link construction |
| Other | all the rest |

**T2. Phases of Run B**, from the timers of the run: wall time, accepted edits, edits each
second, number of world exchanges, number of elections. Phases: collapse, surface split, interior
split, repartition, quality, alternation, final finish.

**T3. Compute against wait, for each rank and each phase of Run B.** Use the sample time stamps
and the phase times in the log.

**T4. Timer coverage.** What share of the wall time do the existing timers cover? Name the
largest functions that no timer covers.

**T5. Peak resident memory**, for each rank.

**T6. Confirm or reject each claim, with the measured number.**

| # | Claim from the earlier read-only audits |
|---|---|
| 1 | In Run B each rank waits 37 to 47% of the run for the slowest rank. |
| 2 | Outside the quality phase there is one edit for each rank in each round. |
| 3 | One round has more than 200 world synchronizations. |
| 4 | Elections cost about 177 µs each in Run B, and this is waiting, not message cost. |
| 5 | The final finish repeats about 520 s of upper-bound proof on every rank. |
| 6 | Interior splits run at about 1024 edits each second. Surface work and quality are the slow phases. |
| 7 | In Run A 72% of the rounds are deferrals, and the proof plus the guidance take most of the time. |
| 8 | Tree containers and memory allocation are a small share (state the share). |

**T7. Callers of the hot functions (from the LBR call graph).**

| # | Question |
|---|---|
| 1 | In the preparation: which top-level steps call the interval arithmetic (`Multiply`, `Add`, `Adjacent`, `Blossom`)? Give the share of each caller. |
| 2 | In the native engine: which callers use the interval arithmetic? Divide into wall upper-bound proof, closest point / normal, and other. |
| 3 | Who calls `__nexttowardl`: the fallback of `Adjacent` (`CNativeAffineReference.cpp:49`) or the direct `std::nextafter` calls in that file? |
| 4 | Who calls `OptionA::InStrip`? How many calls for each guidance evaluation? (The loop is at `SU2_CFD/include/adaptation/CNativeBLTarget3D.hpp:407-413`.) |
| 5 | Who calls the `BoundaryGuidance` lambda, and from which engine step (seed scoring, candidate trial, census)? |

## 6. Rules

- Numbers come from the profiler and from the logs of these two runs. No proxies. No estimates
  given as measurements.
- Give the sample count behind each share. A share from fewer than 100 samples is "not resolved".
- Stop only for these two causes: the solver run does not start, or the flat sampling of
  section 4 fails. Then report. Do not rebuild. Do not change the config.
- A failure in the optional call-graph step is not a stop cause. Do not terminate a run for it.
- Let each run reach its normal end, so that the final timers and counters exist.
- Keep the two new case directories with provenance (program hash, config hash, input hash,
  command lines, host load, resource receipts).

## 7. Report back

- One file in `Handbook/`, 150 lines at most: the tables T1 to T6, the overhead check, and each
  thing that you could not measure.
- Small evidence under R (folded profiles, scripts). No raw `perf.data` in git.
- One line in `HANDOFF_Codex.md` and in `HowToProceed.md` that points to the report.
- Then stop. Do not start the design brief.
