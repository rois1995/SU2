# FP2-D, continuation 2: step D makes no edge below the hand-over limit

Author: reviewer, 2026-11-23 (design review and source check by Codex). For the main agent. The user sends it; it
is a user order. Copy this file with its SHA256 into `R/night_20261115/FP2_D/inputs/`.
Code state: the checkpoint of the second failed lab, `3c494fac91` (candidate of build657, kept) = S0 of this
continuation. "The order" = `FAST_PATH_STAGE2_D1_REV2.md` with `FP2_D_CONTINUE_1.md`. This text continues them;
where they differ, this text wins. The reference of the lab stays the lab of FP2-C part 2 (build655). The
helper-watchdog rule of the order holds. Short names as in the order.

## 1. Why

Continuation 1 removed the first failure: N1 passed two times (7 marked sweeps of step D in place of 40, 8,636
splits, 92 waits, no face left, 59,476 final faces in place of 75,954, none above the tolerance).
N6 trial 1 then failed another gate: 5 core edges with a metric length below 1e-2 on rank 4 (smallest 0.0025).
Your diagnosis is correct: no wiring error. What the rows show, and what they do not show:
- the 5 edges are surface edges between two split points, 80 to 324 micrometres long, along the span, at 1 to 2 %
  of the chord near y = 0.78 to 0.80. The metric asks about 30 mm there. Lengths of 642, 321, 161 and 80
  micrometres at the same places and 2 accepted splits in each of the sweeps 7, 8, 9 of rank 4 (pass 1) are
  consistent with a repeated halving: an edge whose pooled sample stays above the limit after a cut is marked
  again in the next sweep, until the floors in space stop it (`Short` at 2 x `Limit()`, rule 2);
- the three end points 104174, 104186 and 104198 of pass 2 were created by step D in its sweep 2;
- the archive does not name the parent edge and the pooled sample of each cut, and it cannot tell a sample that
  did not converge from a walk on another facet. Which step first connected each of the five pairs is not
  verified.
Continuation 1 says that the pooled samples give no guarantee of a falling error or of a count of sweeps. The
short edges of the archive are consistent with that risk.
The floors of step D are lengths in space. The gate of the hand-over is a METRIC length (no core edge below 1e-2).
So this continuation gives step D the hand-over limit as a floor: step D itself then makes no edge below it.

## 2. The change (production; only with `distance_error_cut` on; with it off nothing changes)

2.1 A named constant `HandoverMetricEdge = 1e-2` in `Core.hpp` beside `ShortEdge`, with the comment: "The limit of
the lab's census over all core edges at the hand-over (since FP2-B): the core should hand no edge with a smaller
metric length to the engine."
2.2 The seed screen (a conservative screen; 2.3 decides). An edge with `Length(edge) < 2 * HandoverMetricEdge` is
treated as an edge below the length floor, at the THREE places where step D tests `2 * Limit()` today:
- in `ChooseDistanceEdge` (the seed test that gives `Short`);
- in `FollowDistancePath`, at the lock / floor checks of the walk (a locked walk, as for the floor in space);
- in `SurfaceDistance`, at the admission of a feature edge with a chord above the limit (the test
  `Distance(points_, use.edge) >= floor`): a feature edge that both parents choose does not pass through the two
  functions above.
Make each metric test after the refusal, border and space-floor checks of that place. Comment: "The two halves of
such an edge would be near or below the hand-over limit. This is a screen: the placed point can leave the chord,
and the two new lengths need not be equal. The test in `SplitSurface` decides."
2.3 `Mesh::SplitSurface` gets one more parameter after `near_row`: `double metric_floor = 0` (0 = no test; update
the declaration in `Core.hpp`). Step D passes `HandoverMetricEdge` when the option is on; step L and every other
caller (the direct surface tests, the C2 fixture helper) keep the default, so their results do not change. The
test: after the row `middle` of the new point is built and before the mesh is changed, compute with the SAME
metric-length kernel as `Mesh::Length` (the census of the lab calls `Mesh::Length`) the length from `middle` to
each distinct vertex of the ring cells, the two ends of the split edge included. These are all the edges that the
split makes. A length that is not finite or cannot be evaluated: the validity refusal of today
(`SurfaceRefusal::Validity`); do NOT use a helper that maps an invalid input to "not shorter". A valid length
strictly below `metric_floor`: refuse with `Near`. In both cases the mesh, the point ids and the overlay are
unchanged (the `Place` call was made, as for rule 2).
Comment: "Rule 2 bounds the children in space. This bounds each new edge in the metric, with the measure of the
census. Each edge that an accepted split of step D makes meets the limit at its commit. A later collapse or a
volume step can still make another short edge: the census of all core edges stays the gate."
2.4 Reasons and counters. Keep `Near` as the split refusal and `Refused` as the reason of the face that is left;
keep `Short` and `NeighborLocked` for the stops of 2.2. Give no reason to a face that step D did not record.
Print only, in `SurfaceCounters` and `PrintSurfaceFields` right after `d_nonfinite`: `d_short_metric` (floor
checks of 2.2 that only the metric test stopped; one count for each evaluated check, summed over the sweeps: a
count of checks, not of different edges) and `d_refused_metric` (splits that 2.3 refused). With the option off
both print 0.
Count each refusal of the test of 2.3 (below the floor, or an invalid length) in `d_refused_metric`. For a
refusal BELOW the floor write one capped, print-only row, built before any commit: `NATIVE3D_LOCAL_NEAR ...
outcome=refused reason=metric_floor` with the id of the proposed point, the ring vertex that blocks, the
coordinates of the proposed point and the measured length (`SplitSurface` fills `near_row`; step D sends it through
its existing capped sink). Do not call the formatter of the committed star (`WriteSurfaceNear`) for it. An invalid
length stays on the `Validity` path (through `refuse(Validity)`, so that the counters and the refused set of
today see it) and gets no such row.
Nothing else changes: the key, the table, the stop of continuation 1, the feature rule, rule 2, the floors in
space, step L, step C.

## 3. Fixtures (hidden tag `[.][NativeFP2D]`)

Expected by the review; verify each by hand in a test comment and pin your derivation (R13 for a difference).
(n) The chain: ONE cell with the line ends (-0.5, 0, 0) and (0.5, 0, 0) and the other vertices (0, 1, 0) and
    (0, 0, 1); the constant tensor 0.0025 times the identity at each point (the metric length of the line is
    0.05); a fake that gives a sample above the limit for the midpoint of each edge on that line (also for the
    halves: it never falls) and 0 for every other sample; a tolerance small enough that the floors in space do
    not act. Expected: the metric lengths go 0.05 -> 0.025 -> 0.0125; three accepted splits in two marked sweeps;
    then the asking faces are left `Short` (0.0125 is below 2 x 0.01); `d_short_metric` above 0; no edge of the
    mesh below 0.01. Derive in the test comment what the chain does with only the floors in space (the control).
(o) The inner spoke: the two cells (A, B, C, X) and (A, B, X, D) with A = (-1, 0, 0), B = (1, 0, 0),
    C = (0, 1, 0), D = (0, 0, 1), X = (0, 0.001, 0.001), identity tensors (orient them positively). Split A-B at
    its midpoint. Rule 2 passes (the physical parents have the third vertices C and D). The new edge from the
    midpoint to X is below 0.01. Give the direct call a `near_row` buffer. With `metric_floor = HandoverMetricEdge`:
    `Near`, `d_refused_metric` 1, the mesh and the next point id unchanged, and `reason=metric_floor` in the
    buffer. With `metric_floor = 0`: accepted.
(p) The feature seed: `Box(1e-4)` with 0-2 as the only feature chord above the limit and every face sample 0.
    Both parents choose that diagonal; its metric length is sqrt(2) x 0.01, below 0.02. Expected: no candidate, no
    marked sweep, no `Place`, and exactly one check in `d_short_metric` (the zero face samples keep the ordinary
    seed check out of the case).
(q) All FP2-D fixtures of build657 again, and the option-off coverage (i) of the order. Their meshes have metric
    lengths far above the limit, so the review expects no change; where a pinned value changes, write the old and
    the new value and the reason in the report. With the option off the two new fields are 0.

## 4. The lab

Section 5 of the order with 3.3 of continuation 1, complete, on the new build: N1, N6, N6P two times each,
N1Invariant, the alarm (both values of N6P; the reference values of build655 are 324 faces above the tolerance
and 601 faces left by step D in pass 2, so the alarm is at 487 and 902). All gates as they are (also the census
"no core metric edge below 1e-2" over ALL core edges, the altitude condition, `sweeps_distance` below 40). The
probe N1Invariant keeps its predicate (it counts accepted splits).
The lab scripts: the two new integers are required in the new rows, compared exactly between the two trials of a
case and with N1Invariant, and zero with the option off; a comparison with an older receipt reads that receipt
with its own field set.
Report beside the reference, beside build656 and beside build657, for each case, pass and rank: the splits of
step D for each sweep; `sweeps_distance`, `waits_neighbor`, `path_max`; `d_short_metric`, `d_refused_metric`; the
faces left by reason; the split refusals; the edge bins of the census and the smallest metric edge with its
length in space and the origin of its two points; the smallest altitude, face angle and edge in space;
`classify_above_tol` and the excess bins; the final faces and points; the bad cells after V by class.
Report the totals of the metric stops, and in the regional report of N1 the counts by region of the
captured faces below. Do not conclude from the final bins how many splits were refused or what the engine can
repair: the run S measures the engine.
For the cause (test code; a report, not a gate): `FP2_D SHORT_FACE` rows. In EVERY pass of a lab trial install
`surface_remaining_sink` before `RunPass` (today the lab installs it only for the last pass). In that callback
(it gets the ids and the reason; it runs before step C, which can remove a point) capture at most 50 faces left
`Short` for each rank and pass: the live oriented face and its facet, the coordinates, the three edge lengths in
space and in the metric, and from a separate lab cache of the `Samples` results of step D the four distances and
the count of samples that did not converge. Update the reason map of the proof only in the final hand-over pass,
as today. The rows describe the faces that are left; they do not prove where a chain started (that needs the
sample of each marked edge for each parent: a later item).

## 5. The order of the work

0. One R4 source check of this continuation with what it inherits. Keep the evidence of build656 and build657.
1. The change of section 2 and the fixtures of section 3. Build `SU2_CFD` and `UnitTests/test_driver` without a new
   warning. Run: all FP2-D fixtures; the option-off coverage (i); all 18 FP1 cohorts; G1 identical
   (375 / 114 / 228). Seal the receipts. Commit and push ("FP2-D continuation 2: fixtures passed; default path not
   yet proven").
2. The lab of section 4. Then the label of the order ("FP2-D: lab passed; default path not yet proven", or its
   alarm / FAILED label).
3. The run S and the close: items 3 and 4 of section 6 of the order, not changed ("FP2-D: S passed; default path
   not yet proven"; the report also has the history of the two failed labs). What comes after that label: as the
   user orders that you have say.
The failed-gate rule of the order holds for each step (keep the code; one read-only diagnosis; one correction and
one repeat only for a wiring error against this text; else the FAILED label, stop and wait).

## 6. Not in this continuation

A test that the error of an edge falls after a cut, or the removal of samples that did not converge from the
table (later items, after the rows of section 4). A change of the samples or of the walk. A metric floor in step
L, in step C or in the volume steps. A change of rule 2, of the floors in space or of the value 1e-2.
