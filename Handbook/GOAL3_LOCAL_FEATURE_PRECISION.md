# M6 local closest-feature precision and strict edge classification

2026-10-17, branch codex/native-3d-bl. Full Goal3 remains ACTIVE. The primary
uncertainty/global complexity initialization experiment is still incomplete.

## Connected numerical correction

The saved first-layer edge18209/61331 rejects before construction edits. Replaying
its original owner-UPPER6 field needs only10 AABB candidates,8 exact closed-segment
donors and one exact source interval. Original node IDs/coordinates/sensors retained;
no donor extrapolation, tensor refit or substituted sensor. The native core is
recomputed over all61330 owner tensors using the same BLAS routine as the backend:
.056676963474795186 versus the raw9 volume replay's .056676963474793819. The latter
is not silently used as the remesher core.

First replay with the current chord envelope still exhausts65536 compositions.
Global rounded closest-point subtraction contaminates a micron-scale displacement.
TriangleDisplacement now computes the same selected closest face/edge/vertex
displacement in local extended precision: signed plane projection for an interior
face, clipped segment residual for an edge, direct local offset for a vertex.
Weights/tangential interpolation, the actual nearest-feature search, the on-wall
normal classification, sensor intersection/finer demands, h0/growth/fade/floor
policy are retained. This is geometric numerical evaluation, not a hard-normal
reset: off-edge/vertex radial directions remain. Active-number builds retain their
prior arithmetic; native3D is primal-only. Active builds were not compiled here.

The local-displacement-only replay still fails65536. It is preserved. Strict
geometry partitions omitted the EXISTING on-wall classification distance
64*epsilon*max(wall_abs_coordinate,wall_extent). That tiny band is now included
for strict faceted3D edges. Provisional scalar quadrature does not acquire it;
analytic geometry and2D retain their partition policy. No quadrature/depth/work
limit, q/J/height/geometry/length/transfer gate was increased or relaxed.

## Exact evidence and limitations

All folders below are in integration_evidence/native_3d_bl_v1/oneram6_uncertainty_v1.

- first_layer_edge_v1: rejected exact source replay;65536 compositions/.732s.
- first_layer_edge_local_displacement_v1: precision-only rejection;65536/.662s.
- first_layer_edge_endpoint_diagnosis_v1: diagnostic point-query export rejects
  where rounding a near-wall segment point leaves the original volume. Retained,
  not a new native remeshing failure. The completed helper uses existing exact
  TraceSegment semantics and original P1 weights, as TargetEdgeLength does. It does
  not authorize an outside-domain point-query fallback.
- first_layer_edge_geometry_bands_v1: strict integral passes with67 compositions,
 4 cuts, length1.1410984442839704, ~.00134s integral/51MiB processRSS. This compares
  a rejected budget-limited calculation with a successful calculation, not complete
  adaptation throughput or a CFD speedup. All source/input/binary/build/helper
  hashes, donor list, cuts, samples and resource receipts are saved.
- first_layer_edge_band_reference_v1:16/32/64 Gauss within declared geometric
  bands expose a small unresolved endpoint switch; not a certified reference.
- first_layer_edge_dense_reference_v1: bulk fixed64 plus1024/2048/4096 midpoint
  samples inside [0,8.657825015520768e-9]. Finest two give1.1410984425910695;1024
  differs6.56e-11. Native difference1.6929e-9 absolute/1.4836e-9 relative, ~1.48
  times its nominal numerical error allowance. Indicators/references are numerical,
  NOT proven enclosures. Do not claim certified1e-9 accuracy or ignore this gap.
  This edge is far below the unchanged1.8 mesh length gate. Reference process
  maxRSS50244KiB, .88s elapsed/.49 user/.04 system, zero swaps.
- first_layer_edge_controls_v1:22 connected3D cases/439753 assertions and8 existing
  2D cases/95 assertions pass. New tests cover translated thin faces, true off-edge/
  vertex radial directions and the strict normal-classification band.
- first_layer_edge_scalar_checks_v1:88/157/329 cold/warm estimates and indicators
  match exactly within each field. Compared with prior point arithmetic, volume
  contributions change at most9e-14; counts remain8192/3668/6092(full)/5644(correction).
  These observations do not transfer an independent volume reference certificate
  to every changed query or establish complete-field accuracy.

Reproduce focused replays using edge.py LABEL [SAVED_CUT_FILE]; edge_checks.py runs
the connected scoped controls and original-cell cache checks. Historical helpers/
exact changed sources are saved beside rejected/qualified stages. Reuse requires
matching provenance; use a fresh label with a changed helper/binary/source.

## Current construction attempt and resources

construction_local_geometry_v1 is terminal REJECTED after91.20s total/80.60s backend.
The original edge is addressed, but the next original surface edge18209/18216
exhausts65536 at piece0/parameter.6446990966796875. No edits/accepted mesh. The
existing wall-normal barycentric thresholds also require strict geometry cuts;
the connected complete partition is being built, not an edge-specific exception.
This is not evidence of successful BL construction.

The one owned MPI4 construction-only attempt used
construction.py reuses the original closed input links/config/saved flow/owner
sensors, injects the SAME provisional field and calls the production backend. No
CFD step or solution transfer; budget normalization accuracy is NOT established.
Native automatic work/300s time/2GiB accounted per-worker memory policy and all
mesh constraints remain;600s launcher envelope includes expensive driver startup.
The process receipt is terminal_process.json; result.json saves the rejection.
Maximum rank processRSS759992320 bytes; sum of rank peaks2944921600 bytes
(~2.74GiB) is an upper bound on peak aggregate rankRSS, not an observed simultaneous
peak. GNUtime reports742180KiB max processRSS,326.54 user+6.34 system seconds,
zero swaps. These contended correctness timings are not a performance comparison.
That build/session33093 is terminal. Current terminal state is recorded below.

One heavy job, nice19, libraries1 thread/build-j1. Local replay address-space limit
2GiB; no concurrent CFD, compilation or MPI task. Before MPI startup~41GiB host
RAM available, initial four ranks~1.5GiB aggregateRSS/~39GiB available. Native tracked
bytes and per-process peaks are not whole-host or aggregate peak RSS. No cluster
submission/profiling comparison is requested.

Whole frozen M6 mass/global calibrated uncertainty, production normalization
coupling, accepted M6 mesh, actual adapted-mesh reassessment, full lifecycle cost
and the deferred query/index/shared-edge/transaction performance package remain
OPEN. This correction alone does not prove the refinement hypothesis.


## Current complete shared edge strategy

The next actual M6 attempt (construction_local_geometry_v1) exposes a SECOND
original surface edge18209/18216. It also exhausts65536 before edits. The complete
strict geometric partition now includes the same1e-10 barycentric thresholds used
by the existing face/edge/vertex normal policy, plus the existing on-wall distance
band and nearest-feature/distance roots. This is one geometric policy, not an
edge-specific dispatch. Candidate classification work is bounded to65536 roots;
nearest-envelope lookup is logarithmic in its already bounded partition.

normal_classification_surface_edge_v1 preserves the negative result: complete
classification cuts alone still fail. Fixed interior rules agree, while the old
endpoint-inclusive rule propagates a rounded normal switch at a cut into its
neighboring large panel. More cuts/depth/sweeps are not the chosen cure.

Composed EdgeLength now uses interior Gauss7/Kronrod15, positive weights on the
same exact original donor intervals and declared geometric panels. The original
global error budget (1e-9*length+1e-12),65536 compositions, depth20, tensor checks,
source tracing and all mesh gates remain. Sensor-only analytic integration is
unchanged. Numerical error estimates remain heuristic, not rigorous bounds.

The initial interior version retained obsolete endpoint-ULP bracketing panels.
It passes both real edges but FAILS an existing efficiency check:23787 accumulated
compositions versus the unchanged10000 test ceiling. This is preserved in
interior_edge_controls_v1 with exact failing source. The complete implementation
removes those obsolete panels, retains each geometric cut once, and passes that
same check. No performance threshold was raised.

Final current-source evidence:
- interior_surface_edge_final_v1: length1.0004777566316599,45 compositions; independent
  fixed64/dense-endpoint reference1.0004777566220813, difference9.58e-12.
- interior_first_edge_final_v1: length1.1410984425864735,75 compositions; reference
  1.1410984425887527, difference2.28e-12. All stage/input/source/binary/helper/
  build/donor/cut/sample/resource receipts saved. References share native point
  tensors/exact source trace and geometric bands, but not the adaptive controller;
  empirical comparisons, not independent tensor-law proofs or rigorous bounds.
- interior_edge_final_v1:29 affected3D tests/439823 assertions,8 existing2D/95,
  three original-cell full/correction cold/warm checks PASS. Includes preserved
  rejection of an unresolved undeclared step, microscopic strong/noisy fields,
  original/extension coverage and full-target balanced split placement. New
  regression checks that endpoint values remain available to Query while their
  measure-zero spike is not spread over adjacent integration panels.

The subsequent MPI4 construction-only attempt in construction_interior_edge_v1
is TERMINAL REJECTED at300s; see the stage/resource result below. No owned job
remains. Its original field/config/all gates are retained; no CFD step, normalized
budget proof or solution transfer follows.

This correction is necessary for the construction experiment. Complete-field
uncertainty/normalization and the broader query/transaction performance package
remain open; local query-count reductions are NOT whole-remeshing speedups.


## Terminal current M6 result and next step

construction_interior_edge_v1 is TERMINAL REJECTED, not an accepted mesh. It
progresses past the two exact edge failures and makes447 global INTERNAL
first-row publications (same aggregate on all ranks),1788 work. It then exhausts
the unchanged300s elapsed budget in first-row initialization. No full candidate
or transfer/CFD advancement; accepted CFD mesh/solution retained. Kernel numerical
blockage is addressed for the exercised queries, not proven absent from every
unvisited edge. No further SU2 run or time/work-limit increase follows this result.

Actual313.213s launcher/300.649s backend. Outside-backend12.565s includes startup/
input/output/finalization, not an isolated measured initialization stage. Per-rank
weights~63.29s, refine~230s, nested first-row~227.45s. Rank2 nested edge geometry
147.86s versus9.37–11.88s on others; its edge total216.44s versus73.55–80.45s.
Rank2 has FEWER compositions (5.12M) than rank0/1/3 (5.94M/6.88M/6.21M). Thus query
count alone does not explain this preparation cost distribution. Source tracing
~8.70–9.78s/rank; composition46.65–54.80s nested inside edge totals. Keep these
nested measurements separate; do not sum rank-seconds/per-stage maxima as a
critical path or label unmeasured residuals MPI wait/geometry. No accepted-mesh
throughput, speedup, scaling or CFD/adaptation ratio follows. These saved counters
help prioritize the already recorded reuse/shared-edge/transaction package.

Max rank process peak759738368 bytes (~725MiB); sum rank peaks2910175232 bytes
(~2.71GiB) is an upper bound, not a simultaneous peak. GNUtime reports1123.21 user+7.16 system seconds,313.20s elapsed and741932KiB
maximum processRSS. The authoritative machine-readable per-rank/stage values and resource receipt are
stage_summary.json/resources.txt, not a new benchmark campaign. Zero process
swaps; host~40GiB available afterward; all owned jobs terminal.

The refinement hypothesis remains INCONCLUSIVE because no accepted complete mesh
is available for reassessment. This is a budget/cost stop, not proof of geometric
infeasibility or an admissible final mesh. The current provisional field is not
normalized to a certified100000 budget. Return to the primary GLOBAL absolute
uncertainty/finite BL-support contribution policy; do not add another local
construction fallback or merely extend300s. Full complexity coverage/calibration
and production coupling remain the immediate incomplete requirement. The broader
performance package is still recorded, not implemented.
