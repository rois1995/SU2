# Larger verified-probe experiment — not adopted

The retained implementation keeps its existing bounded100× recovery. Adding a1000×
start recovered one unavailable application derivative under the unchanged verification
test, but did not demonstrate a qualified practical application benefit. Both experimental
C++ files were restored exactly to the retained source. Candidate source, binaries, tests,
debugger traces and unsuccessful runs remain preserved.

## Evidence and numerical contracts

On the unchanged d27 executable, return tracing identified the first failed coupled SA
product after34 feasible probes and34 successful residual refreshes. The two refinement
searches exhausted without a verified derivative. This excludes a boundary/residual
helper failure for that event; it does not distinguish cancellation from nonsmoothness.
A second debugger experiment changed only that product’s starting probe:1000× and10000×
starts returned a verified derivative, while the original start failed. Earlier accepted
attempts matched exactly. Debugger timings are ineligible.

The candidate tried bounded100×/1000× starts, skipped duplicate clamped starts and kept
the same5e-4 derivative test, relative step cap, feasibility, full live residual refresh,
base-local reset, true-defect limit, acceptance criteria and rollback. No option or vector
was added. An independent mass-null-mode derivative detector at probe1e-10 failed four
availability checks on the retained implementation;96 other checks passed. The candidate
passed130 focused assertions on1×1,1×4,2×2 and2×4, all61 native tests on both primary
layouts,91 ordinary tests, and species-boundary, residual-failure and turbo CLI regressions
on both layouts:13 jobs, all successful. Test success did not establish application benefit.

## Matched application work

All rows below use1×1. Configuration hashes match within each pair. Every endpoint is
**unqualified** and every timing is **ineligible due to contention**. Prefixes stop at the
specified update count. Full trials have a nominal300-second limit; their work and failure
times describe different unfinished trajectories, not time to convergence. Native SA151
states and work were unchanged on both layouts.

| Case / update budget | Version | Updates | Flow / turb assemblies | Krylov | Retries / backtracks | Fresh density RMS | Loop / elapsed seconds | Peak rank KiB |
|---|---|---:|---:|---:|---:|---:|---:|---:|
| SA coupled / forcing / 6 | Retained | 6 | 838 / 1004 | 160 | 3 / 0 | 4.05286e-13 | 20.044 / 20.660 | 139972 |
| SA coupled / forcing / 6 | Candidate | 6 | 1655 / 2074 | 415 | 2 / 0 | 3.71495e-13 | 43.283 / 43.830 | 176888 |
| Nozzle segregated / 20 | Retained | 20 | 2006 / 400 | 716 | 2 / 3 | 4.83158e-06 | 16.326 / 16.784 | 67588 |
| Nozzle segregated / 20 | Candidate | 20 | 2008 / 400 | 716 | 2 / 3 | 4.83158e-06 | 16.041 / 16.499 | 67560 |
| SST coupled / 20 | Retained | 20 | 3382 / 3818 | 430 | 3 / 0 | 4.56138e-07 | 82.086 / 82.650 | 175800 |
| SST coupled / 20 | Candidate | 20 | 3365 / 3801 | 430 | 3 / 0 | 4.56142e-07 | 85.907 / 86.466 | 175436 |
| SA coupled / forcing / full | Retained | 72 | 12210 / 14907 | 2605 | 36 / 0 | 1.91396e-13 | 303.705 / 304.270 | 142232 |
| SA coupled / forcing / full | Candidate | 34 | 11320 / 14199 | 2844 | 16 / 0 | 1.96994e-13 | 300.353 / 300.929 | 177260 |
| SST coupled / full | Retained | 216 | 12074 / 13496 | 1415 | 4 / 0 | 1.70756e-07 | — / 301.041 | — |
| SST coupled / full | Candidate | 227 | 12499 / 13969 | 1463 | 4 / 0 | 1.6626e-07 | 300.218 / 300.727 | 175892 |

SA’s full maximum paired point error was3.15691e-5 before and3.29226e-5 after,
above1e-5. Its lower retry count came with more Krylov work per accepted update.
SST’s matched20-update work barely changed. Its full candidate retained the same
CFL3.90625 after four retries and missed density by five orders of magnitude.
The old SST timeout’s physical report is from a saved checkpoint; its216 updates
and work counters describe the last observed attempt, not that checkpoint’s exact state.
Its missing final loop timer is left blank. Neither full trial supports a speed claim.

## Reference check and reproduction

A separate native SST check tightened the density history stop from-12 to-13:
258 to397 updates. Both satisfy the fresh physical residual checks, but their maximum
component state difference3.34996e-6 exceeds1e-6. The258-update native state therefore
cannot establish definitive state agreement. Additional native checks converged at545
updates (history-14) and reached the stricter history-15 wall limit at1403 updates.
397 versus545 has maximum component difference3.80e-7;545 versus1403 has3.26e-8,
both within1e-6. The545-update state is now pinned for conservative SST audits. The
1403-update trial remains a stopping failure. Thresholds and raw records are unchanged.

Archive: `newton_krylov_refactor_20261015/application_qualification_20261018/` beside
the feature checkout. Exact commands and hashes: `coarser_application_commands.json`,
`coarser_sst_commands.json`, `coarser_candidate_checks/commands.json`,
`coarser_final_comparison.json`, `sst_reference_stability.json` and
`coarser_candidate_not_adopted.json`. Debugger commands and interpretation are under
`product_return_trace/` and `typed_product_probe/`. The independent detector is under
`coarser_detector/`. The companion CSV pins every result/config/binary/source manifest.

Retained d27 CFD SHA256:
`7424b9a9003e0ba0a178c9b757347733a7c812a7254bf54c1cd576f8e2aa54c5`.
Rejected candidate CFD SHA256:
`3e91763e1195922eab731dee40890713dc02c10416e5e52102bb3c8ce5394438`.
The candidate manifest and `source.patch` reproduce the uncommitted experiment.
The build directory still contains that candidate; retained control runs explicitly
use the immutable `recovery_release` executable. Broad control qualification and
quiet repetitions remain outstanding.
