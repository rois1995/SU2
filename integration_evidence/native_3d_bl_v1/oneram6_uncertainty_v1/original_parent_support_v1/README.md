# Original-parent support qualification

Base c70bf64e2b, codex/native-3d-bl; full Goal3 ACTIVE/incomplete.
[Implementation/proof and current status](../../../../Handbook/GOAL3_ORIGINAL_SUPPORT.md).

Canonical serial core:70 cases/14,417 assertions. canonical_mpi passes first-row,
subset ownership, atomic/work/clock, curved/reader controls. canonical_runtime passes
actual RANS/SA+BDF N4/M2 and independent mesh/transfer/restart/SU2/CGNS audits.
`canonical_frozen_comparison.json`:219,532 unique actual M6 query rows are bit-identical
(sensor AND composed tensor) against forced full imports; actual private geometry is
byte-identical to both fallback and the saved legacy first-row replay. The fallback
uses uniformly shifted original donor IDs, retaining coordinates/values/canonical
ordering. This comparison is exact in its saved scope, not universal bitwise claims.

Initial pre-canonical traces failed strict equality with~one-ULP sensor changes.
`frozen_comparison.json`, `frozen_residuals.json`, source capsules and before-canonical
helper remain. One-donor Barycentric weights differed from general exact segment
weights; existing TraceSegment now supplies both. No numerical/mesh tolerance raised.

`m6_stage_summary.json` and ../construction_parent_support_v1 contain one changed
actual complete frozen M6 replay:10,080 private parent replacements, still rejected
at300s before seed/volume edits. Donor import~.09s/rank; metric ranking~88--265s,
including geometric edge preparation14/12/192/106s. Do not claim accepted/full-event
speedup from rejected progress or the older different-build first-row receipt.

The initial combined coupled run passes ordinary/grouped/cold construction and
removal, but the generic finer-sensor case exhausts4096 work. The unchanged baseline
fails at the same4096 work/162 rounds and produces a byte-identical rejected finer
grid; baseline_finer_comparison.json records attribution. No limit change or passing-
suite label is justified. Completed
cases/source/binary/helper receipts remain independently inspectable.

One owned heavy job, nice19/build-j1/thread1/MPI<=4,4TB compile-tmp; host~38--41GiB
available and swap unchanged. Runtime process maxima are not simultaneous MPI peaks.
No CFD advancement or cluster run. No additional tensor cache, geometry fallback,
original-donor refit, adapted-node field, hard-normal reset or test-specific mode.
