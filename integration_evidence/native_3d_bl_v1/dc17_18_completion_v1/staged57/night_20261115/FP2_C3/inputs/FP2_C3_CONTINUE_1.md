# FP2-C part 3, continuation 1: step C makes no edge below the hand-over limit; three measurements

Author: reviewer, 2026-11-24 (design review and source check by Codex). For the main agent. The user sends it; it
is a user order. Copy this file with its SHA256 into `R/night_20261115/FP2_C3/inputs/`.
"The order" = `FAST_PATH_STAGE2_C3.md`. This text continues it; where they differ, this text wins. The five
readings of your source check of the order are accepted. Short names as in the order.
Code base of this continuation: `ae9ef407fa`, the kept candidate of build663 ("FP2-C3: lab FAILED; default path
not yet proven"). Work from that checkpoint with its build and fixture pins. In the reference checks and helper
variables that the order gave you, `S0` STAYS `0c6860fd3a` (build660, with the sealed FP2-D lab and the passed run
S of FP2-D). Build663 is the second baseline of the reports.

## 1. Why

The lab of build663 shows that the contact tests work: 38 rank-pass checks, no embedding violation, no contact
failure; N1 and N6 passed two times. Step C now removes 21,960 / 23,184 / 22,411 surface points (5,682 / 6,889 /
6,827 at build660).
N6P trial 1 failed one gate: one physical edge, (69215, 74584), with the metric length 0.00905 (0.456 mm) on
rank 0 in pass 2. Your diagnosis is correct: both points were there before the pass, so only a collapse of step C
can have connected them. No wiring error.
The cause is in the tests of step C: test 9 (`Size`) bounds the NEW edges of a collapse only from above. Nothing
bounds them from below. Step D leaves groups of points with a spacing just above the limit (its metric floor,
FP2-D continuation 2); a collapse in such a group can connect two points that are nearer than the limit.

What this continuation assumes, and what supports it:
| Assumption | State |
|---|---|
| The short edge was made by a collapse of step C | derived from the rows of build663 (your R5 diagnosis) |
| A collapse p -> k makes exactly the edges from k to the vertices of the new cells that had no cell with k | from the source (`MeasureCollapse`; the link test before it) |
| A refused collapse leaves the mesh as it was | from the source (`PrepareSurfaceCollapse` changes nothing) |
| Step L and the volume steps make no edge below the limit | NOT TRUE in general: no test bounds their new edges from below. How often it happens: NOT KNOWN. Measured here (2.3) |
| The count of collapses that the new test refuses is small | NOT KNOWN. Counted here (2.2). Not a gate |
| Where the 785 s of test 11 on N1 go (1 refusal in 72,564 queries) | NOT KNOWN. Measured here (3.1). No change of test 11 in this continuation |
| Why a pooled sample of step D does not fall after a cut | NOT KNOWN. Rows here (3.2) |

## 2. The change (production; only with `outward_collapses` on)

With `outward_collapses` off: the decisions, the meshes, the counters and the modeled admission of today do not
change. Where `PrintSurfaceFields` prints a row, the four new fields are 0. An option-off comparison with an
older field set compares the old fields exactly and checks the four zeros apart (extend the option-off rows of
the fixture driver so; an exact dictionary comparison must drop the four fields after the zero check).
2.1 The smallest new edge. `Mesh::Plan` gets `double shortest = infinity` (the start value). `MeasureCollapse`
evaluates each `Length({keep, p})` of its loop one time, as today, and uses that value for the maximum of today
(`score`) and for the new minimum (`shortest`). Keep the order of the evaluations and what happens at an
exception. It makes no decision from `shortest`; the volume path does not read it.
2.2 The test, in `PrepareSurfaceCollapse`, right after the two returns for `Refusal::Size` and `Refusal::Shape`
and before `ExactlyPositive`: when `options_.outward_collapses && attempt.plan.shortest < HandoverMetricEdge`
(strictly below, as the census), return through the existing `refused(Reason::Size)` path (a fresh early trial,
as each early refusal; no new enum value) with a new member `short_metric = true` of `SurfaceCollapseTrial`
(default `false`). A direction that passes the test keeps its score. In `CollapseSurface`, in the existing
report-only loop that counts the refusals of the two prepared directions, count such a direction also in a new
field `c_refused_metric` of `SurfaceCounters` (it is in the count of `Size` too, as every `Size` refusal).
Comment at the test: "Test 9 bounds the new edges from above. This is the lower bound, with the `Mesh::Length`
and the strict limit of the hand-over census. A collapse that replaces p by k can add edges only at k, and by the
link test an edge that k already has to a vertex of a new cell is in `incident`: so the test covers each new
edge. Step D leaves points with a spacing just above the limit; without this test a collapse can connect two of
them below it (N6P, build663). A short edge that is there before the collapse can stay, and steps L and V have no
such bound: the census of all core edges stays the gate."
2.3 Three counts for the cause (print only). In `Mesh::SurfaceWork`, when `stats.report &&
options_.outward_collapses`: count the different live edges of this rank with `Length(edge) < HandoverMetricEdge`
right after step L, right after step D, and right after the place of step C (before `MarkUnresolved`; when step C
is off, copy the count after D). New fields of `SurfaceCounters`: `std::optional<size_t> short_after_l{0}`,
`short_after_d{0}`, `short_after_c{0}` (`c_refused_metric` is a `size_t` with the start value 0).
Use one scoped result of `Edges()` at a time, after the tables of the step are gone; it uses the edge workspace
that the pass already admitted. Make NO collective admission call inside `SurfaceWork`. Keep the counts out of
the clocks of the steps. A count must not change or stop the pass: a count that ends stores its number; a count
that throws stores `std::nullopt` and writes one diagnostic line with the checkpoint and the cause where that is
possible. Print each of the three explicitly, as its decimal number or as the word `unavailable` (never 0 for a
count that failed).
2.4 Print the four new fields in `PrintSurfaceFields` right after `c_contact_fresh_max` and before
`seconds_c_contact`, in the order `c_refused_metric`, `short_after_l`, `short_after_d`, `short_after_c`.
Nothing else changes: the contact tests, test 11, the index, the checks of 3.6 and 3.7 of the order, steps L and
D, the volume steps.

## 3. Measurements in the lab (test code only; reports, not gates)

Attach them only in the lab trials of this continuation (N1, N6, N6P). Their rows are parsed for the report and
are NOT in the exact comparison of two trials. Keep one pass of sums, a fixed list of ten, and at most 100 face
rows for each rank and pass. Report the seconds and the scratch of the observers apart from the measured calls;
the production clocks of step C contain the observer in a lab trial (write that beside them).
3.1 Where the time of test 11 goes. `LabBoundObserver::WithinTolerance` times only the call that it forwards. For
each call it records the key of the face, the canonical facet and its entity, the longest edge in space and the
answer; for the ten slowest calls also the oriented corner coordinates, captured at the call. Build the lookup
from id to slot one time for each pass, at the first call (L and D are done then). Bins are half-open; the last
bin is open. Report for each case, rank and pass:
- the calls and the seconds in the bins below 1 ms, 1 to 10 ms, 10 to 100 ms, 100 ms and more;
- the five entities with the most seconds: calls, seconds, mean longest edge;
- the calls and the seconds by the longest edge in space: below 2 mm, 2 to 5, 5 to 10, 10 to 20, 20 mm and more;
- the calls and the seconds in three groups. Match the calls, the committed children and the final physical skin
  by the sorted point ids with the stored facet binding (keep the canonical facet apart, for the entity). Keep
  the calls and the seconds for each key; when `surface_collapse_sink` fires, mark the last matching call. At the
  end of the pass: group 1 = all calls of a key that is live in the physical skin after `RunPass` (the volume
  steps keep the skin); group 2 = the marked calls of a committed child whose key is not live any more; group 3 =
  all other calls. The strict re-checks of the existing collapse probe are not calls of this sample;
- the ten slowest calls: entity, corners, seconds, answer;
- where the lab can read it WITHOUT a production change and without a second strict query: `direct` of the face
  (as in the `LAB_ABOVE` rows of part 2) and the work units of the query, with the calls and seconds for
  `direct=1` and `direct=0`. Write "not available" where it cannot.
When the bookkeeping of an observer fails, mark that measurement "unavailable"; the forwarded geometry answer, a
geometry exception and the existing callbacks must stay as they are.
3.2 The faces that step D leaves `Refused`. The `SHORT_FACE` rows of FP2-D continuation 2 captured nothing: a
split of step D that the metric floor refuses returns `Near`, and `SurfaceDistance` records the attempted edge as
`Refused`; `Short` is only the screen before a split (an error of that text). A face counts as `Refused` only
when `surface_remaining_sink` gives that reason in the last census of step D (a face that reaches a refused edge
through a path can get another reason). Keep the `SHORT_FACE` observer.
In each rank and pass of a lab trial attach `surface_refusal_sink` and `surface_near_sink` beside
`surface_remaining_sink` (keep an existing callback of the invariant mode). For the step `surface_distance`
record the sorted ids of the attempted edge, the refusal and an ordinal of the pass; keep for each edge the last
refusal with its ordinal and "a metric-floor row matched it". A near row of step D with `reason=metric_floor`
belongs to the `surface_distance` refusal callback right before it, and that refusal must be
`SurfaceRefusal::Near` (the near row names the trial point and the blocking vertex; the attempted edge comes from
the refusal callback).
In the remaining callback (before step C) capture at most 100 faces left `SurfaceRemaining::Refused` for each
rank and pass as rows `FP2_D REFUSED_FACE`: the content of the `SHORT_FACE` rows (the live oriented face and its
facet, the coordinates, the three edge lengths in space and in the metric, the four sample distances from the lab
cache and the count of samples that did not converge) and, for the three edges of the face: the last matching
refusal by ordinal (or "none"), and 1 or 0 for "a metric-floor event of step D of this pass names this edge".
Keep the sample cache to the end of the remaining callbacks (also the samples that the last census asks first);
look up the oriented face key before you sort the ids for its edges; a sample that is not in the cache is
"unavailable" in the row; make no extra sample query. The reason map of the proof stays as it is (final pass
only).
3.3 For N1 and N6P: rows `FP2_D REFUSED_REGION` with the true rank and pass. Apply the wing and centroid bin
rules of the regional report to the CAPTURED rows of 3.2; count the captured faces outside the bins apart; and
give `total_refused` = all `Refused` callbacks of that rank and pass before the cap. Print zeros for an empty bin
and for a rank and pass with no such face. This report does not depend on `chosen_invariant` (do not call the
existing `WingDistanceRegions::Report` for it: that one prints `rank=0`).

## 4. Fixtures (hidden tag `[.][NativeFP2C3]`)

Derived from the source by the review; verify each by hand in a test comment and pin your derivation (R13 for a
difference).
(k) The floor. The serial flat tent `S33(0)` of the contact fixtures: removed point 5 at (1, 1, 0), keeper 6 at
    (2, 1, 0), the keeper protected by the fixture geometry, tolerance 0.5, `shape_test = false`. The new
    neighbours of 6 are the points 0, 4, 9, 21, 25 at the distances sqrt(5), 2, sqrt(2), sqrt(2), sqrt(3): the
    smallest is m = sqrt(2). Build a fresh mesh and geometry for each arm; call `CollapseSurface` directly
    with `report = true`; with the option on give it a fresh `LocalContactIndex` and `ContactContext{index,
    nullptr}`. Read the `Size` count from `c_refusals[static_cast<size_t>(SurfaceCollapseRefusal::Size)]`. For
    "the overlay does not change" use `TentOverlayGeometry` with a fresh overlay.
    - Tensor (0.009 / m)^2 times the identity at each point, option on: 5 -> 6 is `Size`, 6 -> 5 is `Protected`,
      the call returns `Size`; `c_refused_metric` and the `Size` count are 1 (one count for a refused
      direction, also when two of its new edges are short); the mesh, the overlay and the point count do not
      change.
    - Tensor (0.011 / m)^2 times the identity, option on: 5 -> 6 commits (6 -> 5 stays `Protected`);
      `c_refused_metric` is 0.
    - The first tensor, option off: 5 -> 6 commits as at build663; `c_refused_metric` is 0 (the control: the
      case bites).
(l) The counts. A serial `BoundMesh` with the one cell (0, 0, 0), (1, 0, 0), (0, 1, 0), (0, 0, 1) and the tensor
    0.000025 times the identity; `surface_splits`, `distance_error_cut` and `surface_collapses` true; a
    non-null fake geometry with zero samples and `Dimension(id) = 0` for all four points; `stats.report =
    true`. The six metric lengths are 0.005 and 0.005 x sqrt(2). Expected: no surface and no volume change;
    the three fields are 6, 6, 6 with `outward_collapses` on and 0, 0, 0 with it off. Check the printed row
    too.
(m) All fixtures (a) to (i) of the order again; for (j) the option-off coverage again. The review expects no
    change of a pinned value: their tensors are far above the limit. Where a value changes, write the old and
    the new value and the reason in the report.
The fixture driver: the serial selector without `NativeFP2C3MPI`, and the selector `NativeFP2C3MPI` with exactly
two ranks. When (k) and (l) are two new serial `TEST_CASE`s, the expected serial count of the driver goes from 7
to 9; the three two-rank cases stay.

## 5. The lab

Section 5 of the order, complete, on a NEW build number with new receipt folders (no output goes into a folder of
build663), with these changes:
- Order of the campaign: N6P trial 1, N6P trial 2, N1 trial 1, N1 trial 2, N6 trial 1, N6 trial 2 (the campaign
  driver has the order N1, N6, N6P today). The failed case then answers first. Stop at the first failed receipt
  of a trial; run the exact comparison of the two trials of each case after all six trials passed alone. Keep
  the terminal-process check and the watchdog logic.
- In each surface row of a trial that ends normally: `c_refused_metric`, `short_after_l`, `short_after_d` and
  `short_after_c` are there as decimal integers that are not negative; `c_refused_metric` is not above the
  printed `c_refused_size` of the row; all four are equal between the two trials of a case. The runner must
  check this itself (the comparator drops a value that is not an integer). A field that is missing or not valid,
  the word `unavailable` included, fails the qualification of the trial: write the rank, the pass, the field and
  the raw value in the receipt. Rows that are missing because a watchdog ended the trial follow the watchdog
  rule of the order.
- The three counts of 2.3 have NO zero condition. The zero condition for short edges stays where it is: the row
  `FP2_B0 edge_bins checkpoint=out` of the LAST pass, for each rank (`out` is after all volume work and before
  `WriteBack` and the shift).
- All other gates of section 5 of the order as they are (the fields of 3.8, the contact rows, the cell digests).
  Read build660 and build663 with their own field sets; write "not available" for what they do not have (the
  new fields, the tables of section 3, trial 2 of N6P of build663).
Report beside the sealed rows of build660 and of build663, for each case, pass and rank: `c_refused_metric`;
`short_after_l`, `short_after_d`, `short_after_c` beside `lt_1e_2` of `checkpoint=out` (each pass) and of the
final `shifted` census where there is one; the first checkpoint that is seen above 0 among the counts that are
there; the smallest metric edge with its length in space and the origin of its two points where the checkpoint
has them; the surface points that step C removes;
and the tables of 3.1 to 3.3. A count above 0 after step L can be an edge that was there at the start of the pass,
and equal counts at two checkpoints can be different edges: name a step as the maker of an edge only where the
rows of that edge show it.

## 6. The order of the work

Work with the FP2-C3 versions of the fixture driver, the lab runner, the comparator, the campaign driver and the
S launcher as they are now. Do NOT copy a build660 helper over them again. Pin this continuation, its SHA256 and
its own R4 source check in the new receipts; keep the pins of the order, the references of build660 and the
evidence of build663.
0. One R4 source check of this continuation with what it inherits.
1. The change of section 2 and the fixtures of section 4. Build `SU2_CFD` and `UnitTests/test_driver` without a
   new warning. Run: the fixtures of section 4; the option-off coverage; all 18 FP1 cohorts; G1 identical
   (375 / 114 / 228). Seal the receipts. Commit and push with the label of this step: "FP2-C3 continuation 1:
   fixtures passed; default path not yet proven".
2. The lab of section 5. Then the label of the order ("FP2-C3: lab passed; default path not yet proven").
3. The run S and the close: items 3 and 4 of section 6 of the order with the existing FP2-C3 launcher, the
   fixture and lab receipts of the new build and the sealed run S of build660 as the reference ("FP2-C3: S
   passed; default path not yet proven"). The report also has: the history of the failed lab of build663; for
   each rank and pass of both events the four new fields (the word `unavailable` stays as text: no conversion,
   no gate in the run S), the existing rows `NATIVE3D_LOCAL_SHORT` at `checkpoint=c1` (after step C, before the
   volume steps) and at `pass=0 checkpoint=shifted` (the final census): `lt_1e_2` for the count and the row
   `order=1` for the smallest edge where it is there; the first checkpoint that is seen above 0. (The run S has
   no census after the volume steps of each pass: write "not available"; the same for a row that is missing.)
   Then STOP and wait for the user, as the order says.
The failed-gate rule of the order holds for each step (keep the code; one read-only diagnosis; one correction and
one repeat only for a wiring error against this text; else the FAILED label, stop and wait).

## 7. Not in this continuation

A change of test 11 or of its place (after the numbers of 3.1). A metric floor in step L or in the volume steps
(after the counts of 2.3). A change of step D, of its floor or of its samples (after the rows of 3.2). A change of
the value 1e-2. A production counter inside the geometry query.
