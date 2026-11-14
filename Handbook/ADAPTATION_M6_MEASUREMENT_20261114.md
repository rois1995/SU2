# M6 measurement-only report — 2026-11-14

**Stopped on profiler failure, as required by the brief. Run A is partial; Run B did not start.**

Evidence root R/measurement_20261114_v1, where R=`integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57`.
Commands, program/config/input hashes and environment: `run_A/provenance.json`; status/resources and profile summaries are retained beside it.
The CFD SHA256 matched `0311b5a2a4f871bd82f48cd7cef3eac49a932cd1d90012bbc7e3c0d887e74ee6` before Run A.
Run A repeats the required test-driver entry point, SHA256 `e6c25520db86bdd212b500249420a7cf3ceddf4af5c17c40e02b1b0b743cd365`.
The saved485 inputs, new492 target, config and 388.8s/233220work/12GiB limits were preserved; all recorded pins remained unchanged.

## Failure and run status

Ordinary-user `cycles:u`/99Hz preflight and streaming leaf sampling succeeded. No full-run call graph was enabled.
At 10,873 native samples, shared range arithmetic/libm exceeded 10% without a clear caller; one 20s DWARF attachment was requested.
DWARF recording exited 0 and captured 1,982 sample records (16,802,896B). Decoding repeatedly emitted:
`addr2line /home/rausa/.debug/.build-id/8b/1dac9de97fa8f6833abad0834b2fb0f3d8ab35/elf: could not read first record`.
The decoder emitted134errors in total; the owned native/profiling workload and failed decoder were terminated. No retry, rebuild or configuration change.
The decoder was terminated with SIGTERM; its error log and failure summary were preserved, then its raw DWARF data was removed.
Run A controller elapsed 298.439s; native sample window 292.332s. No final native timer/edit result was emitted.
All owned run handles were verified terminal; the test-driver SIGTERM assertion is an operator stop, not a discovered engine failure.

## T1 — exclusive identified leaf groups

Run A, rank0 only: 28,898 native samples, CLOCK_MONOTONIC 134934.461582672–135226.793814401; 9 other-PID/pre-exec samples excluded.
These cover process preparation plus partial surface work, not a completed stage. Sampling shares are CPU samples, not elapsed-time shares.
Shared geometry arithmetic/libm stays in Other because callers could not be decoded. Inlined child costs are not charged to a parent.
Thus proof/target/tree/etc. rows count identified symbols only, not complete inclusive costs; classification/self-checks are in `summarize_samples.py`.

| Group | Samples / 28,898 | Share |
|---|---:|---:|
| Wall upper-bound proof | 119 | 0.41% |
| Exact predicates | 1288 | 4.46% |
| Target / BL guidance | 7876 | 27.25% |
| Tree containers | 456 | 1.58% |
| Memory | 473 | 1.64% |
| MPI library | 1 | not resolved (<100 samples) |
| Pack / unpack | 64 | not resolved (<100 samples) |
| Cavity / topology | 84 | not resolved (<100 samples) |
| Other | 18537 | 64.15% |

Run B ranks0–5: no samples; not measured.

### Top 20 symbols, Run A rank0

Abbreviations G=`SU2NativeGeometry::(anonymous namespace)`, P=`SU2Native3D::(anonymous namespace)`; full demangled names/DSOs are in `run_A/samples_summary.json`.

| # | Symbol (signature abbreviated) | Samples / 28,898 | Share |
|---:|---|---:|---:|
| 1 | dc10::OptionA::InStrip | 4392 | 15.20% |
| 2 | G::Multiply(Range,Range) | 4316 | 14.94% |
| 3 | libm::__nexttowardl | 3681 | 12.74% |
| 4 | TargetPolicy::BoundaryGuidance lambda | 3412 | 11.81% |
| 5 | G::Add(Range,Range) | 2434 | 8.42% |
| 6 | G::Multiply(Range,long double) | 2254 | 7.80% |
| 7 | G::Adjacent | 1529 | 5.29% |
| 8 | G::Blossom | 1249 | 4.32% |
| 9 | P::Coordinate(double) | 393 | 1.36% |
| 10 | P::Difference<66> | 275 | 0.95% |
| 11 | P::Difference<132> | 209 | 0.72% |
| 12 | P::Product<66,66> | 188 | 0.65% |
| 13 | G::NormBound | 169 | 0.58% |
| 14 | G::EdgeNormal | 166 | 0.57% |
| 15 | G::UnionChord | 157 | 0.54% |
| 16 | G::Difference(Range[3],Range[3]) | 153 | 0.53% |
| 17 | libc::__memset_avx2_unaligned_erms | 141 | 0.49% |
| 18 | G::TriangleDistance | 133 | 0.46% |
| 19 | libc::__memmove_avx_unaligned_erms | 132 | 0.46% |
| 20 | SU2Native3D::PlanarReference::Get | 129 | 0.45% |

## T2 — Run B phases

| Phase | Wall time / accepted edits / edits per second / world exchanges / elections |
|---|---|
| Collapse | Not measured: Run B unstarted |
| Surface split | Not measured: Run B unstarted |
| Interior split | Not measured: Run B unstarted |
| Repartition | Not measured: Run B unstarted |
| Quality | Not measured: Run B unstarted |
| Alternation | Not measured: Run B unstarted |
| Final finish | Not measured: Run B unstarted |

## T3 — Run B compute against wait

| Ranks | Phases | Compute / wait |
|---|---|---|
| 0,1,2,3,4,5 | All seven requested phases | Not measured: no Run B samples or phase timestamps |

## T4 — timer coverage

Not measurable from this interrupted run: only the surface-entry cumulative timer was emitted (elapsed7.649453481s, accepted0, rounds0).
The last saved trace has elapsed140.997519710s, round640, accepted106; this is a partial receipt, not a final result.
There is no final native timer or exact phase boundary for the sampled interval, so neither coverage nor uncovered wall seconds can be calculated.
Largest caller-ambiguous leaves are G::Multiply(Range,Range) 4,316 samples (14.94%) and __nexttowardl 3,681 (12.74%).
Whether their callers are covered by existing timers is unresolved; they are not asserted to be untimed functions.

## T5 — resident memory

Run A rank0 observed VmHWM maximum 2,237,336KiB (2.134GiB); sampled VmRSS maximum 2,032,524KiB.
VmHWM was polled every0.5s; termination left GNU time empty. This is an observed high-water lower bound, not a certified final/whole-job peak.
Run B ranks0–5: not measured.

## T6 — claims

| # | Assessment and measured evidence |
|---:|---|
| 1 | Unresolved: Run B unstarted; no 37–47% wait measurement or slowest-rank attribution. |
| 2 | Unresolved: no completed Run B rounds/edits; no per-round claim verified. |
| 3 | Unresolved: no Run B world-exchange/round counts. |
| 4 | Unresolved: no Run B election timing or wait/message attribution. |
| 5 | Unresolved: no final finish in either run; no 520s proof measurement. |
| 6 | Unresolved: no Run B split/quality phases; no 1024 edits/s measurement. |
| 7 | Unresolved: Run A has no final deferral/round counters; target/BL identified leaves 7,876/28,898 (27.25%), proof119/28,898 (0.41%), caller-ambiguous Other18,537/28,898 (64.15%). |
| 8 | Partial Run A only: trees456/28,898 (1.58%); memory473/28,898 (1.64%); combined929/28,898 (3.21%). No inclusive/full-run or Run B conclusion. |

## Overhead and remaining gaps

The >5% native-time/edit comparison cannot be made: Run A was interrupted before final counters; Run B was not started.
Reference A remains305splits/389.57s native; reference B remains3640.7s/736323tets. No new completion, count difference or overhead correction is claimed.
Run B would use pinned build494 per brief; the historical293 reference used an older test-driver workflow. No runtime comparison was made.
Profiler files stayed below2GB: largest recorded simultaneous run-profile footprint 27,928,299B; retained measurement files about16,991,355B.
Host/load/RAM/ownership receipts are preserved. Runs used nice19, one native process, and single-threaded libraries; no second heavy run started.
Full-run profiles, phase rates, wait causes, exact final memory, timer coverage and all unresolved claims remain unmeasured.
No source/build/gate/budget/limit/config change or refactoring recommendation. Measurement stopped; design brief was not started.
