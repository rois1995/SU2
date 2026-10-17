# Exact shape dominance before first-row edge measurement — validated bounded stage

Base ca5d3184a0b1dace3f4c720e7c776699aa30682e, codex/native-3d-bl.
Full Goal3 ACTIVE/incomplete. One owned heavy job/nice19/build-j1/thread1/MPI<=4.

## Missing capability and complete rule

The existing first-row initializer ranks candidates lexicographically: maximize
min(q/q_min,J/J_min), then minimize maximum full-target edge length on EXACT shape
ties. It previously integrated all edges before checking whether shape could win.
The last bounded M6 stage spent about10 summed rank-seconds evaluating metrics for
64 initializations, despite reduced MPI coordination. This is a constructor-work
measurement, not accepted full remeshing or a CFD-ready lifecycle comparison.

MetricShapeQuality shares MeasureMetricCells' validation and actual-centroid
arithmetic through one private implementation. Its only public result is the
scalar ranking score; no incomplete PatchMetric can escape to a mesh certificate.
Only a candidate STRICTLY worse than the current successful incumbent skips full
edge measurement. Equal/better candidates retain the original full source-resolved
composed edge integration, comparison, geometry/height/positivity and eventual
strict q/J/full-edge/transfer acceptance. No numeric screening tolerance, bounds,
new candidate, target smoothing, hard normal reset or testcase dispatch is added.

Initial shape scores never certify an edge or unsupported composed integration.
An incumbent exists only after successful full measurement. The target remains
frozen during ranking and integration; every actual centroid query composes BL,
including the full measurement's query after screening. Original sensor-only P1,
finer demands, core/fade and noise0 remain unchanged. Query counters retain the
additional actual shape evaluations; screen/prune counters show saved candidates.

## Validation contract

- Shared scalar/full q/J score is exact, with invalid/duplicate/uncovered inputs
  rejected. A shape score alone cannot authorize a composed field missing its edge
  policy. Strictly worse candidates prune; ties still need full lengths.
- Original full M6 field/all303359 donors and wall geometry, same130 working cells
  and64-work private stage as first_row_transactions_v1. Batch1/8 at MPI2/4 must
  match each other AND the previous selected geometry/markers/volume/outer skin
  exactly. No reduced support, donor refit or new field normalization.
- Source/build/binary/input/helper pins, existing first-row/atomicity/general batch
  controls, curved return and independently audited small RANS/SA N4M2 current+BDF/
  SA/restart/SU2+CGNS. One connected campaign, resumable by complete cases.
- Keep memory/load/resource receipts; no unchanged180/300s full-driver or CFD rerun.

The copied field/config/geometry inputs are links closed within versioned evidence.
first_row_rejected.vtu remains an inspectable partial working subset, not the
complete wing, accepted final mesh, solver-ready flow or final quality certificate.
General difficult construction, original-field uncertainty/reassessment/correction,
composed gradation, viscous accuracy and complete lifecycle affordability stay open.

## Results

67 serial cases / 5,208 assertions pass, including actual-centroid score identity,
invalid source/geometry and missing composed-edge policy. MPI4 reservation,
first-row and general batch controls pass. Curved runtime return and independent
RANS/SA N4/M2 construction/removal, current+BDF conservation, positivity, original
sensor transport, restart and SU2/CGNS geometry/target reload pass. Those are the
existing small scoped workflows, not converged M6 flow. The composed-field audit
still reports 44 violations (maximum ratio 24.5184); this change does not correct
or certify global gradation.

All four direct M6 cases (MPI2/4, batch1/8) match the previous constructor's exact
selected geometry, physical markers, positive rational volume and complete outer
skin. All use the same 303,359 original donors and wall authority. Of 256 candidates
in 64 private initializations, 181 skip full edge measurement. Fresh edge integrals
fall 1,856 -> 951 (48.8%). Cache hits also fall 1,728 -> 99 because many measurements
are avoided altogether, rather than replaced with cache accesses. A lower cache-hit
count therefore does not imply more work. No sampled target or candidate was changed.

| Ranks | First-row seconds, previous -> screened | Metric rank-seconds, summed | Native kernel seconds, max |
|---|---:|---:|---:|
| 2 | 6.04936 -> 3.43293 | 9.55537 -> 5.45528 | 7.39091 -> 4.67987 |
| 4 | 3.58462 -> 2.07742 | 9.98318 -> 5.77774 | 4.45663 -> 3.03486 |

First-row elapsed is 43.3% / 42.0% lower; summed metric work is 42.9% / 42.1% lower.
These are single local observations at batch8, not repeated statistical or MPI
scaling comparisons. Native time excludes CFD initialization, metric normalization,
partition weighting/migration, mesh return, transfer and solver rebuild. Full-event
affordability and accepted difficult construction remain unproved. Full details,
checked previous receipts and input equality are in comparison_previous.json.

## Rejected assertion, resources and next work

The first serial pilot stopped at the old requirement that this box fixture must
produce edge-cache hits. The screen correctly prunes its later measurements, so
that old mechanism assertion no longer applies. The test now requires actual
screening/pruning AND successful full edge measurement; its exact geometry, height,
input preservation and atomicity checks remain. Independent scalar-edge reuse tests
still pass. The failed log/provenance/result, executed source patch and pilot resource
receipt are retained under serial_controls_rejected_obsolete_cache_assertion and
initial_*. No production change was made after that failed control.

One owned heavy job, nice19, build-j1 and single-threaded libraries throughout.
Initial build 6m35.40s; test-only refresh 1m02.79s. Largest recorded build process
2,514,592 KiB (~2.40 GiB); pilot process 662,828 KiB (~647 MiB), not total-job peaks.
Pilot took 22.87s; the resumed campaign took 41.79s and reused completed MPI2 cases
only after checking binary/source/build/input provenance. A live MPI4 process-tree
RSS sample was 940,249,088 bytes (~897 MiB), including shared pages repeatedly; it
is a snapshot, not peak/PSS. Host available memory stayed about 39–40 GiB and host
swap usage did not grow; GNU time records zero swaps. Per-rank native byte admission
is unchanged (maximum 1,161,490,808 at MPI2 / 580,996,152 at MPI4). Resource receipts
and the sample remain here. All owned build/test handles are terminal.

This closes the measured first-row query/publication follow-up; do not add another
cache or unchanged full-driver run on the strength of this isolated gain. Return to
global integration-error calibration/allocation and original-field reassessment,
then address the difficult construction's missing capability. The full Goal3 scope
and unchanged acceptance gates remain active and incomplete. No 2D/MMG path changed.
