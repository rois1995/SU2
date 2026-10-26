# Diagnostic55 changelog — 2026-10-26

Base commit: 5f469f7800; numerical implementation remains e9263060cf.
Primary handover: [M6_DIAGNOSTIC_DECISION.md](../../../../Handbook/M6_DIAGNOSTIC_DECISION.md).

- Extended only the hidden NativeFrozenVolume3D test with optional rejection capture:
  at most twelve copied patch pairs per rank, written after the event. Saves cells,
  physical facets, sparse parameter bindings, failed-cell diagnosis and reason.
  Default diagnostics are off. No production acceptance, source, partition or MPI
  implementation changed. The capture callback's allocation/copy overhead is real;
  per-run work counters match, but bytewise rejected states were not compared.
- Added weak, Linux/GNU test-only PMPI begin/end hooks. The optional external collector
  records local callsite aggregates with fixed memory, no added communication or
  barriers. It reports time inside MPI, not network time. No root privilege is needed;
  execution required the environment's ordinary MPI sandbox escalation.
- Added extraction/reconciliation, offline weight descriptors, independent centroid
  witness checks and aggregate analysis using existing NumPy and frozen-field AABB
  facilities. No dependency installed. Full mesh/sensor binaries and compiled profiler
  remain local, with hashes and reconstruction commands instead of binary git payloads.
- Preserved the initial witness-analysis failure: comparing the reported first failed
  cell with the complete patch minimum was incorrect. Source inspection established
  the streamed measurement semantics; the corrected per-cell comparison reproduces
  all sixteen shape failures. No production numerical change was required.

Validation performed:

- Single-worker test-driver build passes, 6:04.87 elapsed; largest single-process RSS
  4,111,976 KiB (not whole-build peak). Build log retained.
- Two-rank PMPI self-check passes exact counts, result values, type widths and timing
  bounds for ten reductions and one broadcast per rank.
- Original mesh/metric extraction checks all 61,330 coordinates/global IDs, SPD values
  and bitwise binary round trip. All 29 baseline terminal-receipt hashes verify.
- Six-rank plain/observed frozen replays both terminate normally at work2048; 476 private
  edits, 879 candidate shape refusals, ten matching selected counters per rank. Neither
  produces an accepted mesh. Native durations89.999/62.723s; host variability prevents
  isolating instrumentation overhead. Per-run source/runtime/input pins verify terminal.
- Independent shape analysis validates28 captures,16 reproduced shape failures and exact
  original-donor containment for every sampled centroid. No source extrapolation used.
- Offline predictor checks cover uniform/anisotropic alignment, rotated equal-density
  targets, joint rotation invariance, SPD/intersection positivity. Current replica sum
  matches production5,664,358; exact individual production weights were not exported.

Unexecuted: complete local repaired-witness acceptance, alternative shape policies,
whole-M6 or BL runs, transfer/restart, broad regression, alternative partitions,
held-out useful-compute prediction. Broad regression was not repeated for test-only
instrumentation; earlier production evidence remains historical with its own pins.
No new performance gain, topology feasibility proof or DC-17/DC-18 closure claimed.
