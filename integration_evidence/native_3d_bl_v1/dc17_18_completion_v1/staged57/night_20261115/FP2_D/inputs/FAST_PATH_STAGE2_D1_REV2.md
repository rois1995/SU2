# Fast path stage 2, item FP2-D (revision 2): step D cuts the edge that carries the distance error (not the longest edge)

Author: reviewer, 2026-11-23 (source checks and design reviews by Codex). For the main agent. The user sends it; it
is a user order. It REPLACES `FAST_PATH_STAGE2_D1.md` (SHA256 `3ed64a17...45a3`) completely: do not read that file
for this item. The design is the same; the references, the fixture set-up, the lab gates and the runners are those
of the code after the two continuations of FP2-C part 2. This order does not stop the work that runs (R12): finish
FP2-C part 2 first. Copy this file with its SHA256 into `R/night_20261115/FP2_D/inputs/`.
Code state: start only after the exact label "FP2-C2: S passed; default path not yet proven" is committed. S0 = that
commit. Write in the progress file S0, its build, its lab acceptance receipt and its passed receipt of run S. With
any other label or a missing receipt: do not start; write the state; stop and wait for the user.
Short names: `Core.hpp` = `Common/include/adaptation/CNativeLocalCore3D.hpp`; `Surf.hpp` =
`CNativeLocalSurface3D.hpp`; `Surface`, `Pass` = `CNativeLocalCore3D<name>.cpp`; `SG` =
`CNativeLocalSurfaceGeometry3D.cpp`; `Run` = `NativeRemeshRun.inc`; `ST` =
`UnitTests/Common/adaptation/CNativeLocalCore3DSurface_tests.cpp` (the probe is in
`UnitTests/Common/adaptation/native_local_surface_probe.hpp`); `VH` =
`UnitTests/SU2_CFD/adaptation/fixtures/native_volume_handover.inc`; `Lab` =
`UnitTests/SU2_CFD/adaptation/CNativeLocalCoreLab_tests.cpp`. Production line numbers are those of `1e7335a71e`
(build654). Test files changed after it: find a test-code place by its name, not by a line number.
The helper-watchdog rule (as `FP2_C1_CONTINUE_2.md`): if a helper watchdog ends a step and every gate that was
evaluated before it passed, keep the attempt and the host load, and repeat that step one time with three times the
helper timeout. If the repeat ends the same way, the step is OPEN for the user: keep the code, stop and wait. The
600 s value of the lab and all comparative timings are reports. The 5400 s gate of each event and the limits of R5
and R6 stay.

## 1. Why

The user looked at the adapted meshes: the leading edge of the wing is remeshed with small ISOTROPIC faces. The
reviewer measured it (run R-S of RCFD1): at the nose the faces are 1.6-2.4 mm across the nose and 2.3-3.7 mm along
the span. The geometry tolerance needs only the size ACROSS the nose (limit 2.4-2.8 mm for a nose radius of 10-14
mm). Along the span the surface is straight: the tolerance permits any length, and the metric asks 21-37 mm. The
input mesh had faces of 6 x 15 mm.
The cause is the rule of step D "a face is cut through its LONGEST edge". At the nose the longest edge is the
diagonal or the spanwise edge. So step D halves the span until the faces are isotropic, and only then the size
across the nose gets smaller. Result: about 19,000 wing faces at x/c below 2 % (1,200 in the input), and step D makes
16,300 of the 18,600 surface splits of event 1.
The samples of step D already say which edge carries the error: the midpoint sample of each edge. This item cuts
that edge. A model of the nose (a cylinder with the triangle pattern of the input) gives 9 faces for each input face
in place of 25, in 5 sweeps in place of 9.
What protected the mesh against tiny edges in FP2-B stays: (1) no other edge is taken when the chosen edge of a face
is locked; (2) a face is cut only through its own chosen edge (the path); (3) a face whose chosen edge is locked is
left; rule 2 (the altitude floor of `SplitSurface`); the length floor, now at each edge of the path. Step D changes
the choice of the edge and tests the floor `2 * Limit()` along the path. Step C runs after step D with its tests of
S0, test 11 (`Bound`) included; on the changed mesh it can collapse other points than on S0. The choice by the
midpoint sample is a heuristic for a surface that is curved in one direction (the nose); on a saddle it can take a
poorer edge. So the lab also looks at the wing tip and at the trailing edge.

## 2. Limits of the item

Production change in step D of the local core (`Surface`, with its declarations and counters in `Core.hpp`), the
switch in `Run` (3.1), and comments only in `SU2_CFD/src/solvers/CSolver.cpp`, `Common/src/CConfig.cpp` and
`config_template.cfg` (3.6). Step L, `SplitSurface`, rule 2, step C, the volume steps, the engine, `Limit()`, the
tolerance and the metric do not change. With only `distance_error_cut` off, each case of S0 keeps its other options
and gives its deterministic results of S0 (clock fields excluded; the five new columns print 0 where the row is
printed). The default path (both config options NO) and the run CORE do not reach the new code. 3.6 changes
comments only: no gate of `shared_engine_regression_gates.json` is needed for it; the report shows that the diff of
these three files has only comment lines. Readable code (D 7.1a): a comment with the reason at the key and at the
path.

## 3. The change

3.1 Switch. Append `bool distance_error_cut = false` to `CoreOptions` after `surface_collapses`
(`Core.hpp:47-58`). Step D uses the new choice (3.2 to 3.4) only when it is set; else it is exactly as on S0. `Run`
sets it beside the three surface options that it sets today (`Run:1015-1021`). `VH` sets it ONLY for the new lab
selectors of FP2-D (N1Invariant included); every selector that exists on S0 keeps its S0 settings.

3.2 The error of an edge. In each sweep of step D, before item 1 and item 2 of the marking, make a sparse table
Err: for each physical face and each of its three edges, the sample at the midpoint of that edge (sample k + 1 of
`Samples` belongs to edge k of `FaceEdges`, `Surface:139-141`; use the sample cache of the step: each face is
sampled in each sweep anyway, the table only makes these calls earlier); Err(e) = the LARGEST finite such sample
over the physical faces at e; store only the edges with Err(e) above `Limit()`. A midpoint sample that is not
finite does not enter Err; count it one time for each face-edge occurrence in each sweep (`d_nonfinite`; the
centroid sample is not counted). The rule of S0 that decides whether a face asks for a cut does not change. The
mesh does not change during the marking of a sweep, so the table is fixed for the sweep. Comment: "The two faces at
an edge can report different samples for its midpoint (the walk starts at each face's own facet). The key of an
edge must be the same from both faces, else the path below can turn in a circle. So the error of an edge is the
largest value over its faces. Which face ASKS for a cut does not change: a face asks when its own samples are above
the limit."
Storage: `SurfaceArrayBytes` has one parameter today (`Core.hpp:274-275`, `Surface:211-224`). Declare
`SurfaceArrayBytes(size_t cells, bool error_table = false)` and charge the table Err only when it is true. Pass
`true` at `Run:977` and `Run:983`. Pass the option of the local `options` at `Lab:692` and `Lab:697`. In `VH` (the
two `SurfaceArrayBytes` calls of the lab pass; both are under a `parallel` choice): with n input cells and
`on = options.distance_error_cut`, the parallel branch adds `SurfaceArrayBytes(n, on)` to the admission and
`SurfaceArrayBytes(1, on)` to its bytes-for-each-cell denominator; the serial branch adds
`SurfaceArrayBytes(n, on) - SurfaceArrayBytes(n, false)` to the admission and the same difference at n = 1 to the
denominator; the numerator stays; with the option off both serial additions are zero. The callers in
`native_handover.inc` keep the default. Write the added bytes in the report.

3.3 The chosen edge of a face. New function `Mesh::ChosenSurfaceEdge(face, Err)` beside `LongestSurfaceEdge`
(`Surface:579-595`); it returns the edge in `ById` order, as `LongestSurfaceEdge` does (the path compares edges
with `==`). The key of an edge, a strict total order (the largest key wins):
  K(e) = ( Err(e) is in the table, Err(e) if in the table else 0, the physical length of e, the smaller sorted id
  pair wins a tie ).
A face with an edge in the table gets its edge with the largest error (equal: the longer; equal: the smaller id
pair). A face with no edge in the table gets its longest edge, as today. A face below the limit, or above it only
by its centroid, can still have an edge in the table when the face on the other side reports a larger sample for
that midpoint; it then chooses that edge. Comment: "On a surface that is curved in one direction, the midpoint of
an edge with the largest extent in that direction has the largest distance to the model; only cuts of the edges
with that extent make the distance smaller. The longest edge can be an edge along the straight direction: its cut
changes nothing. The choice by the midpoint sample is a heuristic; the centroid sample and the feature chords
stay."

3.4 Use in step D. `ChosenSurfaceEdge` replaces `LongestSurfaceEdge` at its three uses in step D:
`ChooseDistanceEdge` (`Surface:623-644`; the feature edge with a chord above the limit still comes first),
`FollowDistancePath` (`Surface:597-621`) and the marking of the feature parents (`Surface:691-718`; a feature edge
is split only when it is the chosen edge of each of its parents, as today with "longest"). The direct test caller
`SurfaceFixtureAccess::DistanceChoice` (`native_local_fixture_access.hpp`) gives an empty table in its present
tests. The structure of the path does not change: from the chosen edge e of the asking face, go to the other face
h at e while the chosen edge of h is not e; stop LOCKED at a refused edge, a border edge or a ring border; the end
is an edge that is the chosen edge of each face at it. Comment at the path: "The key of an edge is the same from
each face at it. The chosen edge of a face has the largest key of its three edges, so each step of the path goes
to an edge with a strictly larger key. No edge and no face repeats, and the path ends before the guard." The guard
of `Surface:615` stays.
The length floor: with the option on, the path tests the floor wherever it tests the locks (each edge after the
first, and in the marking of the feature parents (`check_first = true`) also the first): an edge shorter than
`2 * Limit()` stops the path as LOCKED (`NeighborLocked` for the asking face; a feature edge is then not marked, as
for a LOCKED path today, `Surface:706-710`). Comment: "With the error key a later edge of a path can be shorter
than the first, so the floor is tested at each lock check." The first edge of item 2 has the test of today
(`Short`).
Locks, reasons and counters as today: chosen edge refused -> `Refused`, border -> `Border`, shorter than the floor
-> `Short`; path locked -> `NeighborLocked`; end not the first edge -> mark the end, `waits_neighbor`. No other edge
of a face is ever taken in place of a locked chosen edge. The order of the splits (marks sorted by decreasing
physical length) and the limit of 40 sweeps do not change.

3.5 Print only. In `PrintSurfaceFields` (`Surface:188-208`), right after `points_before_c` and before `c_changes`
and the other quoted fields: `d_error_edges` (the sum over the sweeps of the size of the table Err),
`d_marks_error` and `d_marks_longest` (an accepted step-D split counts in `d_marks_error` when its edge is in the
table of its sweep, else in `d_marks_longest`; their sum is `splits_distance`), `d_centroid_only` (asking faces
with no edge in the table, summed over the sweeps), `d_nonfinite` (3.2). The fields of part 2 (`c_refused_bound`,
`seconds_c_bound`, `c_bound_queries`) stay where they are. Collect the D counters under `stats.report`; with the
option off the five fields print 0.

3.6 Documentation, comments only (no change of behaviour; the user asked that this difference is written in the
code). (a) At the head of the normalisation part of `CSolver::ComputeMetric` (before the definition of
`complexityError`):
```
/*--- What ADAP_COMPLEXITY targets. Two orders of operations exist in this function:
 *    Native remesher (NATIVE_CAVITY, 2D and 3D): for each trial factor the sensor metric is scaled and
 *      bounded and then graded; the factor is solved so that the complexity (the integral of the metric
 *      density) of the GRADED metric meets ADAP_COMPLEXITY, within the tolerance of the solve and where the
 *      metric bounds permit it (see the warnings below). So the target counts the graded metric, and the
 *      gradation decides how much of the target the sensor gets. Observed on ONERA M6 (Mach sensor, norm 2,
 *      ADAP_HMIN 3 mm): with ADAP_HGRAD 1.3 the sensor metric held 9.8k of 50k at the first adaptation and
 *      2.7k of 50k at the second; with ADAP_HGRAD 1.6 it held 27k and 18k. So the size that the sensor asks,
 *      for example across a shock, depends strongly on ADAP_HGRAD.
 *    MMG remesher: the factor solve targets ADAP_COMPLEXITY for the bounded, UNGRADED sensor metric, subject
 *      to the metric bounds and the convergence of the solve; MMG applies its own gradation after that, which
 *      can make the mesh larger than the target.
 *    ADAP_COMPLEXITY is a metric complexity, not a point count.
 *    Both orders are in use in other codes. Decision of 2026-11-23: the native remesher keeps the first
 *      order, so that the target holds for the metric that the mesh follows. Do not change one order into
 *      the other without a new decision. ---*/
```
(b) In `Common/src/CConfig.cpp` at the option `ADAP_COMPLEXITY` (`:3287-3289`) replace the words "i.e. of the
adapted mesh" by this short form, and in `config_template.cfg` add it as a "%" line above `ADAP_COMPLEXITY= 10000`
(`:2885-2886`): "Native remesher: the complexity of the metric AFTER its gradation. MMG remesher: the complexity of
the sensor metric BEFORE the gradation of MMG (the mesh can be larger)."

## 4. Fixtures (hidden tags `[.][NativeFP2D]`, so that the case table of G1 stays identical; serial)

Set-up: step C is OFF in the new fixtures (a) to (h). (a) uses the present cylinder set-up (steps L and D). (b) to
(h) drive step D directly (`SurfaceFixtureAccess::DistanceWork`), with the requested value of `distance_error_cut`
set on the mesh. A fake geometry for step D alone keeps the `WithinTolerance` of `FlatGeometry` (it throws) or
overrides it with a throw; a wrapper forwards the real function.
(a) the cylinder fixture of FP2-A (`ST`, its refused rim edge {1, 2} stays; its option-off result today is 27
    step-D splits and one face left `Refused`) with the option on: the probe of section 5 finds 0 violations; the
    count of step-D splits is below the count with the option off; at the end, every face above the limit is a
    face left with the reason `Refused` (`left_distance == left_refused`, and `left_refused > 0`, as with the
    option off);
(b) the side of a cylinder with n = 8 sectors and m >= 2 rows of height b = 2.4 a (a = 2 sin 22.5 deg; error
    1.09 `Limit()` at tolerance 0.1), flat caps with zero samples, the rim edges as features with `Chord` = the
    midpoint error: give the builder `Cylinder()` and its fake a parameter for m rows and the cap height m b, with
    NO refused rim. Run step D directly. Expected by the reviewer's derivation: sweep 1 splits the n m diagonals
    (an exact tie with the across edge; the longer wins), sweep 2 the n (m + 1) across edges (rims included);
    `sweeps_distance=2`; side faces 2 n m -> 6 n m, cap faces 2 n -> 4 n; no edge along the axis is cut. Verify
    this by hand in a test comment; if your derivation differs, write it in `QUESTIONS_FOR_REVIEWER.md` (R13) and
    pin your derivation. Pin no value that you did not derive;
(c) a face whose edge with the largest pooled error is refused and is NOT its longest edge, while its longest edge
    is free: the face is left `Refused`; no other edge of it is cut;
(d1) the three fixtures of FP2-B correction 1 (`ST`, the three cases after the skewed-box helpers; keep the border
    set-up of their partition mesh) with the option on and their present centroid-only samples, also through the
    direct `DistanceChoice` caller with an empty Err: the table is empty; the edits, the reasons and the counters
    that exist on S0 equal those with the option off; check the new D counters apart (`d_centroid_only` can be
    above 0);
(d2) the path of fixture (a) of correction 1 (`SkewedBox(false)`) with a fake that returns, by the sorted face
    key: face (0, 1, 2): the midpoint sample of 0-2 = 2 `Limit()`; face (0, 2, 3): the midpoint sample of 2-3 = 3
    `Limit()`; face (2, 3, 6): the midpoint sample of 3-6 = 4 `Limit()`; every other sample (also of new faces) 0.
    Expected by the reviewer's derivation: the attempts are 3-6, 2-3, 0-2 in the sweeps 1, 2, 3; `path_max=2`;
    `sweeps_distance=3`; no face left; `path_splits=0`. Verify by hand as in (b), with the same rule for a
    difference;
(d3) the locked path: the same fake on `SkewedBox(true)`: face (0, 1, 2) gets `NeighborLocked` (the ring of 2-3 is
    a border); face (0, 2, 3) asks for 2-3 itself and `SplitSurface` refuses it with `Border` (this refusal comes
    before `Place`); no `Place` call;
(e) a face above the limit only by its centroid (fake samples), and no face at its edges reports a midpoint sample
    above the limit: its longest edge is chosen, as with the option off;
(f) the edge key: (f1) three faces in a fan around one point, each face preferring by its OWN samples the next
    radial edge (the surface of one tetrahedron: point p and the faces (p, a, b), (p, b, c), (p, c, a)): with the
    pooled Err there is no throw; use three different pooled errors; the first split is the radial edge with the largest pooled Err; the
    asking face that does not hold that edge waits (`waits_neighbor >= 1`, `path_max = 1`); derive the exact edits
    in a test comment; (f2) a common edge that one face reports at 0.99 `Limit()` and the other at 1.01 `Limit()`:
    the first face does not ask; the edge is in the table and is the chosen edge of both; it is split in sweep 1;
    (f3) a path whose second edge is shorter than the floor: the face at the short edge is left `Short`; the asking
    face is left `NeighborLocked`; no `Place` call;
(g) an exact tie of the error between an across edge and a diagonal: the choice is the longer edge; work list on
    and work list off give the same edits;
(h) the distance variant of the flat rule-2 fixture of FP2-B correction 2: refused `Near`, as today;
(i) with only `distance_error_cut` off: run the existing surface fixtures of FP2-A, FP2-B and FP2-C again with
    their S0 options (step C on wherever S0 has it on; the corrected serial, border and exchange cohorts of part 2
    included: the pinned `Bound` refusal on S33 with its three equal snapshots, then the removals-only exchange
    with the test wrapper). Their deterministic results equal those of S0, and the five new fields are 0 wherever
    the row is printed.

## 5. The lab (after the fixtures; BEFORE a full run)

Run N1, N6 and N6P two times each with the production `RunPass`, their lab inputs and options of S0, and
`distance_error_cut = true` (new selectors, 3.1). Reference for each case: the measurement of repeat 1 of the lab of part 2 at S0, loaded with the S0 version of
`compare_lab.py::load_trial` (follow its recorded watchdog repeat when there is one); that measurement must have
passed, and its path and SHA256 must agree with the `artifacts` map of the lab acceptance receipt of S0.
Runner and comparator: copy the S0 versions of `FP2_C2/run_c2_lab.py` and `FP2_C2/compare_lab.py` into `FP2_D/`.
Replace their build647 / build652 references and their prerequisites of the C2 continuations by the pins of this
order, the FP2-D fixture receipt and the qualified S0 trial receipts. They read the proof row of rank 0:
`FP2_A LAB_S_PROOF` for the serial case, `FP2_B proof` for the parallel cases. Extend `observed_failures` of the
runner so that a failure that is already visible in the new altitude rows, D counters or N1Invariant rows is a
result failure and not a reason for a watchdog repeat; on a normal end, every row that a gate needs must be there.
Conditions (gate), for each trial:
- each cell exactly positive; `Check`; the donor scan 0; the state build accepts and (6 ranks) the bytes of
  `Snapshot()` are equal;
- the complete qualification of part 2 at S0: `qualified=1`, `classify_unexplained=0`, `model_gap_entities=0`,
  `strict_checked` equal to the count of physical faces of the rebuilt state, `strict_sink_mismatch=0`, and zero
  violations of the collapse probe (`strict_above` included). A face of the sink of step C counts as a face
  without a step-D reason. `strict_mismatch` is a report;
- no core metric edge below 1e-2 at the end; the smallest altitude over ALL live physical faces, recorded right after
  `RunPass` for each rank and pass together with `Limit()` of the geometry (new test code): for the last pass of
  each rank it must be finite and not below that `Limit()`; `star_max` not above 1,000; `sweeps_distance` below 40;
  `c_sweeps` below 50; `v_limit=0`;
- each surface row has the five new fields, not negative, with `d_marks_error + d_marks_longest ==
  splits_distance`;
- both trials of a case qualify and are equal in the exact rows of the part-2 comparator (the `FP2_C cells` digests
  included) plus the five new integers. The report-only fields and rows of part 2 stay report-only
  (`strict_mismatch`, the face totals by entity, the bound refusals by entity and sweep). No row and no mesh is
  compared with S0 as a gate.
A pass above 600 s is a report beside the host load.
The probe case N1Invariant (a NEW serial selector; one time; gate): the inputs and options of the FP2-D case N1
(steps L, D and C on), the same qualification as N1 (donor scan included), and the chosen-edge probe as its only
additional gate; do not use the path of the old invariant selector (it leaves out the donor scan and has the old
walk-probe gate). The probe (test code; a copy of `SurfaceLongestEdgeProbe` with the chosen edge in place of the
longest edge) forwards the complete geometry interface, `WithinTolerance` included (attach it as the bound
observer of the C2 lab is attached). It records `Samples` by the oriented face key during step D only: recording
starts at the first `Samples` call in `RunPass`; each completion row `surface_distance` of `PassStats::sink` resets
the flag "first `Place` of the sweep"; recording ends at the zero-candidate row of step D or at sweep 40, before
step C can sample. At the first `Place` of each sweep of step D it rebuilds Err, independently of the production
table, from all live physical faces of `mesh.Cells()` and the recorded samples. A placement counts only when
`Accept` follows it. Each edge that step D splits must be the chosen edge of each physical face at it:
`checked == splits_distance`, violations 0, no throw of the path guard. Its integer fields of
`NATIVE3D_LOCAL_SURFACE` equal those of the first qualified FP2-D trial of N1. Use the same rebuild for
fixture (a).
The alarm (what the engine gets; not a correctness gate), for each qualified trial of N6P, all six rows of pass 2
required: two values are compared apart with the same value of the reference trial: `classify_above_tol` of the
row `FP2_B proof` of rank 0 (final rebuilt faces), and the sum over the six ranks of `left_distance` of the row
`NATIVE3D_LOCAL_SURFACE` of pass 2 (faces that step D left, before step C). Each must not be above the larger of
(1.5 times the reference value) and (the reference value + 25). If a reference value cannot be read: write "alarm
not evaluated", do not make the run S, commit the evidence, stop and wait. If the alarm is hit: do not make the
run S; commit the evidence ("FP2-D: lab alarm; default path not yet proven"), stop and wait.
Seal in the lab receipt that the run S needs: the gate results of each trial, the N1Invariant receipt and the
result of the alarm.
Report beside the reference, for each case, pass and rank: the surface faces and points at the hand-over;
`splits_distance`, `sweeps_distance`, `path_splits`, `path_max`, `waits_neighbor`, each `left_*`, `refused_near`,
the new fields of 3.5; `classify_above_tol` and the excess bins; the fields of part 2 (`collapses_surface`,
`c_refused_bound`, `seconds_c_bound`); the bad cells by class and the cells; the smallest metric edge over all
core edges; the shortest edge in space, the smallest face angle and the smallest face altitude over all physical
faces (new test code).
And for N1 only, the regional report (test code, from the final surface, faces of the marker `WING3D` only;
b = 1.1963; face centroid; eta = y / b; x/c from the planform x_LE = y tan 30 deg, x_TE = 0.8059 + y tan 15.8 deg).
The counts of step-D splits by kind and region come from the probe run N1Invariant: count each accepted split of
step D one time if at least one parent facet belongs to `WING3D`, in the bin of its edge midpoint, as "error" when
the edge is in the rebuilt Err of its sweep and as "longest" when not. The faces left above the limit come from the keys of
`surface_remaining_sink` of the last pass, matched to the physical faces that still exist after step C, without
the keys of the sink of step C (the same adjustment as the proof); keep the strict census of the final surface
apart from these sampled counts:
- the count of faces with x/c below 0.5 %, 0.5-2 %, 2-10 %, above 90 % (all for eta below 0.98), and of the tip cap
  (eta at or above 0.98), each with its count of step-D splits by kind (error / longest) and its count of faces
  left above the limit;
- at the stations |eta - e0| < 0.03, e0 = 0.2, 0.5, 0.8, for x/c below 0.5 %: the median size "along" = the largest
  |edge . t| with t = the leading-edge direction (sin 30 deg, cos 30 deg, 0) projected into the face plane and
  normalised, and "across" = the largest |edge . (n x t)| with n the face normal.
For a new measurement that S0 does not have, write "not available" and use the archived evidence of S0; do not
run S0 again. Predictions of the reviewer's model (not a gate): the
faces at x/c below 2 % fall to about 40 % of the reference, with a size of 1.3-1.9 mm across and 6-15 mm along.
If a gate fails: the failed-gate rule of section 6.

## 6. The order of the work

1. One whole-item source check (R4). Then the change of section 3 and the fixtures. Build `SU2_CFD` and
   `UnitTests/test_driver` without a new warning. Run: the FP2-D fixtures (a) to (h); for (i) the option-off
   coverage of S0 with the corrected serial, border and exchange cohorts of part 2; all 18 FP1 cohorts; G1
   identical (375 / 114 / 228). Seal the receipts. Commit and push ("FP2-D: fixtures passed; default path not yet
   proven").
2. The lab of section 5. Commit and push ("FP2-D: lab passed; default path not yet proven").
3. One run S (both options YES): copy the S0 version of `FP2_C2/run_fp2_b_g5.py` into `FP2_D/` and run mode S.
   Pin the passed `run_receipt.json` of the run S of part 2 at S0 (follow `full_watchdog_retry` if the passing
   receipt was sealed there); verify its build, config and artifact hashes against the evidence of part 2; require
   the FP2-D fixture and lab receipts (the passed alarm included). These replace the build647 and C2 receipt checks
   of the launcher. Keep its standing gates and its helper-watchdog logic. The S0 hashes are for provenance and
   for the report. Config `FP2_C1/run.cfg`, SHA256
   `07b91a94996bf3cb48352041d8e1643ffe17a61bed07432085cfcf87b1725382`. Work folder `FP2_D/G5/build<N>/S/`. All
   standing gates of both events. The six files are not a gate (the mesh changes by design); write their SHA256.
   Report beside the run S of part 2 at S0 (its receipt and report; not build647), for each event: the native
   maximum; the cells, points and proof faces of the final mesh; the rows `NATIVE3D_LOCAL_SURFACE` with the new
   fields; the engine's collapse pass (seconds, rounds, `collapse_accepted`); the engine seconds of construction,
   of the quality phase and of each geometry repair (the first repair shows what step D left); q / J at
   `quality_entry`; the table "where the time is".
4. Commit and push the evidence ("FP2-D: S passed; default path not yet proven"), the report
   `R/night_20261115/FP2_D/reports/FP2_D_<build>.md`, the progress file, `HANDOFF_Codex.md` and `HowToProceed.md`.
   Verify that no owned job runs. After S passed: STOP and wait for the acceptance decision of the user; start
   neither a default proof nor a second qualification run. Before S passed, the helper-watchdog rule and the
   wiring-error repeat hold.
The failed-gate rule of this item (this user order replaces the revert of R5 for this item): keep the code. For a
failed gate make one read-only diagnosis pass (R5); only a wiring error against this text gets one correction and
one repeat; if the repeat passes, go on. Else commit the evidence with "FP2-D: <step> FAILED; default path not yet
proven", update the progress file, verify that no owned job runs, STOP and wait.

## 7. Not in this item

A change of step L. A size from the wall curvature in the metric. A new collapse rule or a swap (step C of S0 stays
as it is; the span size of the input stays where step C does not act). A new shape limit for the faces (rule 2
stays as it is). A change of `Limit()` or of the floor value. A change of the order of operations of the metric
(3.6 is a comment only).
