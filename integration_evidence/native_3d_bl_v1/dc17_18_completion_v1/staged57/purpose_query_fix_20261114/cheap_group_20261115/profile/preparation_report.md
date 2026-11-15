# Prepared same-v2 A/B profile

Status: prepared only. No profile, build, test, or CFD job was launched. The runner refuses to start until `build_status.json` says terminal/successful, dynamically reads both binary hashes, checks all 1540 source pins and the binaries before and after each case, and records input/helper hashes. Run A must finish successfully with all owned handles terminal before Run B can start. The primary workflow must finish its ordered numerical checks before either profile is launched.

`run_profile.py A` repeats the saved C492 serial case and `run_profile.py B` repeats the six-rank Euler50k case/config from measurement v2, into fresh `profile/run_A` and `profile/run_B` outputs. It preserves nice 19, one-thread libraries, perf at 99 Hz on `CLOCK_MONOTONIC`, optional 60-second LBR captures, and the existing config and source inputs. It does not modify source or old measurement evidence. Use `analyze_profile.py A|B` after the corresponding run; it invokes the existing v2 sample and B phase analyzers, then writes facet resolution and memory receipts.

## Profile method and source mapping

- The old profiler’s SHA checks were pinned to the previous build. The local copy now checks the runtime hashes supplied by the runner from the terminal build record. Sampling rate, clock source, event, parser, and LBR commands are unchanged.
- The inherited 2 GB profiler limit remains 2,000,000,000 bytes with its 1,900,000,000-byte abort threshold. Accounting includes historical `measurement_20261114_v*/run_*/profile` artifacts plus both new run profile directories. Current historical artifacts total about 1.37 GB, leaving roughly 0.53 GB before the abort threshold. A full B profile may hit the unchanged cap; the run will report the profiler failure and stop. No historical files were moved, compressed, or overwritten.
- GDB hooks were source-checked against the current pinned source: Run A uses the `SU2Native3D::PrepareSurface` function at [CNativeEngine3D.cpp:6016](/media/rausa/4TB/SU2_Versions/SU2_NativeIntegrated/Common/src/adaptation/CNativeEngine3D.cpp:6016). Run B uses `SU2Native3D::AdaptMetric` at [CNativeEngine3D.cpp:6026](/media/rausa/4TB/SU2_Versions/SU2_NativeIntegrated/Common/src/adaptation/CNativeEngine3D.cpp:6026), with the existing epoch line breakpoint at [CNativeEngine3D.cpp:1176](/media/rausa/4TB/SU2_Versions/SU2_NativeIntegrated/Common/src/adaptation/CNativeEngine3D.cpp:1176). Line 1175 creates `started`; line 1176 is immediately after it and the GDB hook reads `started._M_d._M_rep`. The adjacent-stepping extraction is in `CNativeAffineReference.cpp` and does not shift this location. The same GDB clock method is retained.

## Required report fields and limits

`analyze_profile.py` reports `facet_resolutions`, `facet_misses`, and `facet_misses - facet_resolutions * 48954` for every available rank/event exit row. It also sums native `process_peak_rss` rows per event and evaluates the 20,000,000,000-byte gate. If a run has no native summary row, it marks that gate unevaluated; GNU `time -v` peak RSS is reported separately and is not silently substituted. The saved C492 v2 log has no `NATIVE3D process_peak_rss` summary row, so the serial run’s inherited native-summary gate is expected to be unevaluated unless the new execution emits one.

## Files

All new preparation files are under this directory: `run_profile.py`, copied/retargeted `rank_perf.py` and `rank_perf_B.py`, the unchanged v2 GDB scripts, `analyze_profile.py`, `method_provenance.json`, and this report. The runner saves before/after runtime/source-pin receipts in each fresh case’s provenance and terminal status.
