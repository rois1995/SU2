# Fast path, stage 0 (item FP0): measure the building blocks on real data

Author: reviewer, 2026-11-19. For the main agent. Base: the accepted state at the time of the order.
All rules of section 1 of `AGENTS.md` apply (R1 to R13). R = `integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57`.

This item is a MEASUREMENT. It adds test code only (R3). It changes no production file (R2), no gate and no reference.
Its numbers decide the design of the next items. No number of this item is a pass / fail gate, except section 5.

## 1. Why

The reviewer designs a new lean local remeshing core (`FAST_PATH_DESIGN_V1.md`; copy it with its SHA256 beside this file and
read sections 1 to 3 for the context; it is not an order). The forecast "about 2 minutes for one adaptation" assumes that one
mesh change costs 10 to 100 microseconds with plain floating point on flat arrays, and that the strict final checks are cheap
when they run one time. Today one accepted change costs 5 to 200 ms. This item measures these assumptions on the real mesh.

## 2. Data and loading (the same for all measurements of part A)

G5 = `R/night_20261115/N4/section6_3_after_N11/round_8_5t_P/G5/full587`.

- Mesh: `G5/adapted_mesh_adap_00001.su2` with `G5/adapted_mesh_adap_00001.su2.native_ref` (146,676 points, 726,442 cells,
  67,170 surface faces).
- Target: the fields `Metric_XX .. Metric_ZZ` of `G5/restart_flow_adap_00001.dat` (the target of event 2 on these points).
  So the data are "an accepted mesh with a new target": the real input of an adaptation.
- Loading: test code, one rank. Use the route of the frozen tests: a driver on `SU2_NATIVE_FROZEN_CONFIG`
  (templates: `UnitTests/SU2_CFD/adaptation/CNativeRemesher3D_tests.cpp:340-404`,
  `fixtures/native_saved_cfd_metric_stage.inc`), the metric from the restart (a frozen sensor file made with the existing
  converter `diagnostic55/prepare.py`, or `driver.ComputeMetric()`), then `ImportGeometry` (`CNativeImport3D.hpp:75-80`).
  Result: `imported.cells` (`Owned`), the donors and `reference.original`.
- Put the new test cases in a NEW test file (short compile time) with a hidden tag that G1 does not select
  (proposal `[.][NativeFastPathStage0]`). The input paths come from environment variables, as in the frozen tests.
- Each measurement prints one line `FP0 <name> {json}` with its counts and seconds (`std::chrono::steady_clock`).
- Run with nice 19, one rank, when no other owned job runs (R1). Write the peak memory of the process in the report.

## 3. Part A: measurements with one rank (test code only)

"Strict" = the production function. "Float" = a twin that you write in the test file with plain `double` and no checks
(no `ValidateTensor`, no exact fallback, no exception). For each row: the count of items, the seconds, and nanoseconds for one item.

| # | Measurement | What to do | Numbers to report |
|---|---|---|---|
| A1 | Load and copy | Time `ImportGeometry`. Then copy `Owned` into flat arrays: unique points (id, coordinates, tensor), cells (4 point indices), the neighbour cell of each face (sort the face keys, as `Incidence` does), a mark for boundary faces and their marker, a mark for surface points | seconds of each part; bytes of the arrays; bytes for one cell |
| A2 | Strict kernels on all cells | `Orientation` for each cell (with `KernelStats`: filtered and exact counts); `Measure` (q, J) with the tensor at the centre (mean of the 4 nodal tensors); the edge length of each unique edge with the closed formula of `CNativeField3D.cpp:465-472` built from `MetricNorm` | seconds, ns for one item |
| A3 | Float twins | The same three quantities with float twins on the flat arrays | seconds, ns for one item; the largest difference to A2 for q, J and length; the count of cells where the sign of the orientation differs (expected 0) |
| A4 | Same number as the gate | For 1,000 edges (fixed stride through the edge list): the closed formula against `FrozenField(..., current_nodal = true).TargetEdgeLength` | the largest relative difference (expected: rounding only) |
| A5 | Work to do under this target | From the float arrays: the count of edges with length > 1.8, < 0.9 and < 0.5; of cells with q < 0.20 or J < 0.05; each count also for "touches the surface" and "interior" | the counts; the seconds of the complete scan |
| A6 | Neighbourhood queries | On the flat arrays: the cells around each point; for each unique edge the ring of cells around it | seconds for all; ns for one edge ring; the largest ring |
| A7 | Lean split (prototype, test code) | On the flat arrays: split each INTERIOR edge with length > 1.8 at its midpoint (new tensor = mean of the two end tensors; each cell of the ring becomes two), longest first, each edge of a pass only if its ring is not yet changed in this pass; check each new cell with the float orientation; repeat passes until no such edge is left or 20 passes | splits, passes, seconds, microseconds for one split; then A5 again on the result |
| A8 | Lean collapse (prototype, test code; only if it fits in the limit of R3) | Collapse each INTERIOR edge with length < 0.5 onto one of its end points when all changed cells stay positive (float) and no changed edge becomes longer than 1.8; one pass rule as A7 | collapses, refused, seconds, microseconds for one collapse; then A5 again |
| A9 | The existing optimizer on large regions | `OptimizeMetricMesh` (`CNativeOptimize3D.hpp:93`) with the production options of the quality stage, boundary of the region fixed, on two regions of the loaded mesh: all cells with the centre inside a ball that holds about 10,000 cells and one that holds about 100,000 cells (choose the centre at a point with many bad cells from A5). Limit: 15 minutes of wall time for each; stop and report at the limit | cells, bad cells before and after, seconds, ms for one cell, moves and reconnects accepted |
| A10 | Strict measurement of the complete mesh | `MeasureMetricCells` (`CNativeMetric3D.hpp:156`) on all cells in batches of at most 128, with a `current_nodal` field of the batch | seconds; ns for one cell; the counts of failed cells (must be equal to A5 for q, J and long edges) |
| A11 | Strict surface proof, one rank | `OwnedFaceReverseBounds` and `GeometryCertificates(budget, true, &bounds)` for the complete surface (template: `CNativeGeometryBackend3D_tests.cpp:1170-1213`) | seconds of each; the work counters; `within_tolerance` of each entity; `mesh_to_model` and `model_to_mesh` |
| A12 | Float surface evaluation | For each surface face: evaluate the cubic patch of its reference facet in float at the face centre and the three edge midpoints (`Curves()` with the float patch functions of `CNativeCurvedReference.hpp:325-483`), and the distance of these 4 points from the flat face. First check the order of the control points on 1,000 samples against `EvaluateSurface` | seconds; ns for one face; the largest and the mean distance; the count of faces above 0.5e-4 and above 1e-4; the result of the order check |

Rules for part A:
- A7 and A8 are prototypes to measure cost. They are not the design of the core. Keep them simple and in the test file.
  If A8 does not fit in 90 minutes of work (R3): report that and continue.
- A9, A10, A11: if a call throws or a limit of the function stops it (for example a patch limit): write the message and the
  place, and continue. That is a result, not a failure of the item.
- Do not change a production function to make it callable. If a function cannot be reached from a test, write it in the report.

## 4. Part B: the cost of the present engine on a mesh that is already good (one heavy run, then one light run)

Purpose: in the new design the present engine checks and repairs a mesh that the core made. Its cost when almost nothing is
left to do is not known. Event 2 of G5 is not this case (its target is new: 4,379 bad cells on rank 0 at the quality entry).

1. Replay event 1 with the frozen-volume test (`[.][NativeFrozenVolume3D]`) at 6 ranks: mesh `G5/m6.su2`, target from
   `G5/restart_flow_adap_00000.dat`. Extend the TEST observer so that it also saves the valid exit state
   (`NativeStageCapture::Save(view, rank, "exit")`; today only a rejected exit is saved,
   `native_transaction_capture.inc:267-268`) with `SU2_NATIVE_STAGE_CAPTURE=1`. This is the one heavy run of this item
   (about 35 minutes, R1). Report: the cells and points at the exit beside the G5 values (726,442 / 146,676).
2. Then run the public `Refine(world, exit_owned, original_donors, geometry, {}, options)` (`CNativeEngine3D.hpp:350`) at
   6 ranks on the saved exit state, with the same options as the production call and `options.timings` on. The target is the
   nodal tensors of the saved state, so the reviewer expects no change of the mesh.
3. Report for step 2: the total seconds; the timer rows of each phase (preparation, construction, quality, final_proof);
   rounds; accepted changes; `valid`; the seed census seconds; the final proof seconds.
4. If step 1 or 2 is not possible with test code in the limit of R3: write why, and report in its place the same rows of
   event 2 of G5 with the note "target changed".

## 5. Gates of this item

1. `git diff` shows no change outside `UnitTests/` and your evidence folder. Build without a new warning.
2. G1: the tables are identical to the reference (the new test cases have a hidden tag).
3. No G2, G3, GQ or G5 run for this item. The heavy run of part B is not a gate run.
4. Commit and push the test code and the report ("FP0").

## 6. Fixed parts from the existing logs (no run)

From the rank logs of `G5` (both events), one table: reference and import; start census; final proof (reverse part and
shared part); return of the mesh (`ReaderSlices`); replace; mesh output (the mesh file and the reference file apart);
`ComputeMetric`. For each: the seconds, and the calls and seconds of `mobility_policy` inside it. Also: how many times
`BuildMobilityPolicy` runs in one event and its seconds for one call (from the timer rows; if the rows cannot show it, say so).

## 7. Report

`R/night_20261115/FP0/reports/FP0_<build>.md`: one table for part A (all rows of section 3, strict beside float), the
results of part B, the table of section 6, each "could not be reached / stopped" case with its message, the peak memory, and
each correction of your test code (R3). Update the progress file. Commit, push, verify that no owned job runs, stop and wait
for the user.

## 8. Not in this item

Any production change. The design of the core. Gates on the measured values. The wall layers.
