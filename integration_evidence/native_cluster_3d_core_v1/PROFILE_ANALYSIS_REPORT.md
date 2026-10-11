# Native3D offline rank-profile summary — 2026-10-11

Repository: /media/rausa/4TB/SU2_Versions/SU2_NativeIntegrated.
Branch: codex/native-3d-core; predecessor ffeeac545762fb95b116fe47c797f3747e8c0d6f.
Status: analysis-only checks PASS; Goal 1 remains ACTIVE.

Published analysis/evidence checkpoint 3e55caee966eef795f60b317a24ba7d948a999a9 on origin/codex/native-3d-core; full remote SHA verified. No executable or run-recipe change.

The previous cluster analyzer retained only outer wall/accepted-work samples for
timing and limits, raw CSV detail, and a status entry for lifecycle campaigns.
It therefore discarded lifecycle paired timings and required manual interpretation
of measured CPU/query/partition/engine costs.

The analyzer now emits rank_profiles for timing/limits and actual lifecycle
campaigns. It retains lifecycle OFF/ON samples and medians, exclusive wall/CPU
stage rows and nested rows in CSV. profile_summary.py uses the rank with the
largest parent wall time for a closed phase breakdown. Each phase also reports
its separate rank distribution; their maxima must never be summed. Nested
details stay separate. Existing field-count checks remain unchanged.

Engine private CPU, attempts/accepted/rejected work, cache/search/interpolation/
composition, donor and partition measures report rank sums/mean/max and max/mean.
Inactive ranks are excluded from engine imbalance. Stages without a worker-count
export leave workers null; they cannot establish the adaptation communicator size.
CPU phase closure is checked when exported. No elapsed-minus-CPU waiting estimate
is produced. For M<N, active_worker_exclusive reports a separate closed worker
row because inactive subset waiting can dominate the global critical row.

The runner, C++ implementation, geometry/metric contracts, frozen targets,
configured driver and original raw evidence are unchanged. Preparation includes
the new helper so future bundles remain closed. Previously prepared jobs can
continue: use the updated checkout for offline analysis when their results arrive.
Do not modify a frozen bundle or combine different package identities into one
paired comparison.

Validation:
- package_checks_v9: 16 lightweight synthetic/mock checks PASS (0.487s unittest
  time, 0.787s wrapper time), Python AST, shell syntax and owned diff checks PASS.
  Checks exercise inconsistent/missing ranks, nonfinite measures, wall/CPU
  closure, the sum-of-phase-maxima trap, active-worker CPU/accepted-work
  imbalance, subset waiting versus worker cost, lifecycle analysis retention,
  fixed-work timing controls, artifact drift and output preservation.
- profile_analysis_replay_v2: PROFILE_REPLAY_PASS on exact pinned historical
  reference_lookup_n4_n4_m2_v1 and profile_runtime_v2 lifecycle MPI4 evidence.
  Retains input/receipt/binary identities and source/output SHA256. Summarizes
  21 native events, 76 exclusive stage events and 84 nested events across two
  original binaries. These remain separate workloads/binaries, not a speedup
  comparison or current-source numerical certificate.
- Earlier checks_v7/v8 and replay_v1 remain as chronological checked drafts.

The historical wedge coarsening event illustrates why both rank views matter.
Global native elapsed is8.527131328s on inactive rank2; its subset wait accounts
for91.00% of that row. Active rank1 has8.526338502s, with7.317050751s (85.82%)
in coarsening and0.321375627s (3.77%) in partition. The separate engine
coarsening row has7.316949562s and71 accepted edits (9.7035 edits/second).
Private metric CPU across the two active workers has max/mean1.2571, reconstruction
CPU1.1307, measurement CPU1.1404 and query-request count1.0379. Those values
do not explain the remaining wall time or establish MPI waiting/communication
cost: the pilot was contended and not a repeated cluster comparison.

Communication exports remain incomplete/self-inclusive; MPI/vendor traffic and
true wait are not calibrated. RSS remains process-lifetime, not event allocation
or ParMETIS/reader/scratch calibration. No runtime overhead change, speedup,
larger-grid result, 3D BL/curved/PDE/unsteady accuracy or affordability is claimed.

Next: review the prepared cluster MPI1/2/4 correctness, lifecycle, three paired
timing repetitions and n8 first-limit pilot. Complete reference-admission fixed-
work/restart qualification and use measured dominant active-worker phases before
choosing further optimizations. Full private/wait/traffic/memory calibration and
practical size/rank limits remain Goal 1 deliverables.
