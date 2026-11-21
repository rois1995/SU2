# Fast path, stage 2, step 0b (item FP2-0b): the float split pass with feature-edge splits, then P3 and P4

Author: reviewer, 2026-11-21 (checked against the source). For the main agent. The user sends it; it is a user order.
Base: `0167ad274f` ("FP2-0 measure float surface splits and strict checks") or later.
All rules of section 1 of `AGENTS.md` apply (R1 to R13). R = `integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57`.
"The text" = `FAST_PATH_STAGE2_STEP0.md` (your copy: `R/night_20261115/FP2_0/inputs/`). Its short names (tol, strict,
float, BP, BPh, AR, ARh, CA, GB), its data (IN1, OUT1), its loading and its rows stay valid where this file does not
change them. Evidence folder: `R/night_20261115/FP2_0B/`. Copy this file with its SHA256 into `FP2_0B/inputs/`.

This item is a MEASUREMENT again. Test code only (R3): no production file changes (R2), no gate, no reference.

## Why

Your report of FP2-0 was complete and your stop was correct. The reviewer's reading of your ledger:
- The 18 faces that fail `ContainsEntityFace` and 34 of the 46 faces with `Classify` above tol are at ONE place: the
  wing root near the leading edge (the original points 64 to 69 and 269 to 273; they are on the curve where the wing
  meets the symmetry plane). The chords of these protected edges are 2 to 8 tol away from the model in IN1. The text
  told you to split no feature edge. Step D can not lower that error; it bisected the other two edges of each root
  face in each pass, until the faces folded (the witness face is perpendicular to the wing; its new points are at
  1/3 and 2/3 of the root chord, 2e-6 from the symmetry plane). This is a gap of the text, not a fault of your code.
  Production does split such edges (OUT1 has 733 traces, IN1 has 674).
- The other 12 faces with `Classify` above tol are wing faces with one vertex on the root curve (4) or on the tip
  section y = 1.1963 (8), none with a feature edge, with a 4-sample value of 0.75 to 0.80 tol. The smallest 4-sample
  value of a face with `Classify` above tol is 0.7501 tol. The float limit 0.8 tol was too high.
- Row P4 did not run. The design needs its numbers.

The one question of this item: with feature-edge splits and the lower limit, does the bulk constructor accept the float
result, and what does the two-sided proof (row P4) say and cost?

CA = `Common/src/adaptation/CNativeCurvedAtlas3D.cpp`, CAh = `Common/include/adaptation/CNativeCurvedAtlas3D.hpp`,
T2 = `UnitTests/SU2_CFD/adaptation/CNativeFastPathStage2_tests.cpp`, W = `.../fixtures/native_fast_path_stage2_walk.inc`,
S = `.../fixtures/native_fast_path_stage2_strict.inc` (your files of FP2-0; line numbers of `0167ad274f`).

## 1. Changes of the prototype (row P of the text; all in test code)

Extend your test files of FP2-0, or copy them; the measurements of FP2-0 need not stay runnable. Keep the hidden tag.

### 1.0 Two things that the prototype needs now
- Budget: `ProposeFeaturePoint` and `RebuildFeatureTraces` take an `AtlasBudget&`. Use ONE
  `Fp2Budget(726442, state.ResidentBytes() + state.Geometry().ResidentBytes(), start, 5400)` for a whole run of the
  prototype, as in `MeasureStrict` (S:403). A refusal with the reason `work_exhausted` or `time_exhausted` stops the
  run: write it.
- Samples: keep the four values of `Walker::Samples` for each face (W:156-170), not only their maximum (T2:301-314):
  [0] = the centre; [k+1] = the midpoint of `FaceEdges(face)[k]`, that is v0v1, v1v2, v2v0 (T2:399-402).

### 1.1 The trace map and the feature test
- Keep a map `std::map<Edge, EdgeTrace> traces` in the prototype. Start: `state.Coverage().traces` of the IN1 entity
  state (674 entries).
- `Feature(edge)` (T2:419-423) becomes: the sorted pair is a key of `traces`. (With the start map this is the old
  answer for each IN1 edge; write the count of IN1 edges for which the two tests differ, expected 0. After a feature
  split, an edge from an original vertex to a new curve point is a feature edge; the old test would say "no".)

### 1.2 The split of a feature edge
For a feature edge (a, b), a < b, with trace `traces.at({a, b})`:
- New point: `SU2NativeGeometry::ProposeFeaturePoint(state.Geometry(), state.Mobility(), {the trace}, p_a, p_b,
  Node{next_id, the flat midpoint}, out, budget, reason)` (CAh:33-35; CA:572-601; production reaches it from
  `BindNewPoint`, BP:1526-1550, call at :1536). `p_a`, `p_b` = the coordinates of a and b; a stored trace runs from
  the smaller id to the larger id. The function gives the binding and the coordinates (`out`). Do not use the float
  walk for this point.
- Refuse and count (`feature_refused`, with the first `reason`): the function returns false or throws; the binding has
  `MobilityDimension != 1` (expected 0: the function tests this itself, CA:597); the binding (support key) or the
  coordinates are those of an existing point.
- Tensor = the mean of the two ends. Split BOTH faces of the edge (they can have different entities: wing and
  symmetry). Each child keeps the anchor of its parent.
- Traces: the two child edges (a, m) and (b, m) get traces; erase the entry (a, b). Make them with production code:
  `SU2NativeGeometry::RebuildFeatureTraces` (CAh:45-47; CA:690-768); production calls it so in `Finish`
  (`CNativeBoundary3D.cpp:308-316`). It reads only `old.surface`, `old.parameters` and `old.coverage`, never
  `old.cells`. A `Patch` with the two old faces, their four points and `coverage.traces = {(a, b): the old trace}` is
  enough. Call it with the four new faces, their five points and `*reference.original`. Take from its output ONLY the
  entries (a, m) and (b, m); keep each other entry of your map. If it returns false, or one of the two entries is
  missing: refuse the split and count it (`trace_refused`, with the first reason). Make the traces BEFORE you accept
  the split, so that a refusal leaves the prototype unchanged. `SliceFeature` is in an anonymous namespace
  (CA:391-530): do not build traces by hand.
- A split of a child feature edge (second level) uses the child trace in the same way.

### 1.3 Step L (length)
As in the text, with one change: feature edges take part (a feature edge above 1.8 is split by 1.2). Cap: 40 passes.

### 1.4 Step D (distance)
Float limit: `0.7 * tol` in the place of `0.8 * tol` (T2:325 and T2:542; add the count "above 0.7 tol" to the counts
of S:323-350). Cap: 40 passes.
In each pass, in this order:
1. Feature chords. For each feature edge whose value is not known yet: `ProposeFeaturePoint` for its flat midpoint
   (as 1.2; no point is made); its chord value = the distance between the flat midpoint and the proposed coordinates.
   Keep the value with the edge (a new child edge gets a new value). A feature edge with a chord value above the limit
   is marked. A refusal: count it (`feature_chord_unknown`), the edge is not marked.
2. Faces. For each face with a 4-sample value above the limit:
   - if one of its edges is a feature edge that is marked in 1: the face marks nothing more in this pass;
   - if one of its edges is a feature edge whose midpoint sample (one of the 4 samples) is above the limit, and the
     samples at the centre and at the midpoints of its other edges are all at or below the limit: the error of this
     face belongs to the feature edge. If that edge is not marked in 1 (chord value at or below the limit, or not
     known, or refused by 1.5): do NOT split another edge for this face; count it (`feature_dominated`: each face at
     most one time over the run, and the count of each pass) and leave it;
   - else: mark its longest edge that is not a feature edge, as in FP2-0.
3. Split the marked edges, longest first (length in space), one change for a pair of faces in a pass, as in FP2-0.
   A marked feature edge uses 1.2; another edge uses the float walk as in FP2-0.
   An edge whose split was refused (a reason of 1.2, 1.5 or 1.6, or a refusal of the walk kind as in FP2-0) is not
   marked again in a later pass; the rules of 1.5 for its faces hold for each of these reasons. A blocked candidate
   (the pair of faces changed in this pass) is not a refusal: it can be marked again.
Stop when no edge is marked, or at the cap.

### 1.5 The float fold check (for each split of 1.2 and of the walk kind, before the split is accepted)
For each of the four child faces and each of its three corners: take the facets `side` of the corner's binding
(`ForFacets`) with `Entity(side) == Entity(the anchor of the child)`; the corner passes when for at least one of them
`dot(unit normal of the flat child, unit normal of state.Geometry().EvaluateSurface(side, binding).normal) > 0.1`
(ARh:175; this is the float picture of `OrientedFace`, AR:1949-1961, which the constructor asks through
`ContainsEntityFace`, BP:305-315). The flat child normal = (p1 - p0) x (p2 - p0) in the vertex order of the child (the
sign of `OrientedFace`, AR:1958). A child with zero area, or a throw of `EvaluateSurface` for a side, is "does not
pass" for that side. If a corner of a child does not pass: refuse the split and count it (`fold`). This EDGE is not
marked again in a later pass, neither in step 1 nor by a face in step 2. In step 2 a feature edge that was refused
here counts as "not marked". A face whose longest edge that is not a feature edge was refused here marks its next
longest edge that is not a feature edge (none left: count the face, `fold_all`). The value 0.1 is a guard of the
prototype, not a gate.

### 1.6 The switch `strict_children` (default off)
With the switch on, after 1.5 and before the split is accepted: `state.ContainsEntityFace(anchor, the three points)`
for each of the four child faces; refuse the split and count it (`strict_entity`) when one fails. Sum the seconds of
these calls. The check runs after the traces of 1.2 are made and before the prototype changes.

### 1.7 What to write for each run of the prototype
- For step L and for step D: for EACH pass the count of marked edges, splits (walk kind and feature kind apart),
  blocked candidates and refusals by reason; the totals; the seconds (the calls of `ProposeFeaturePoint` apart, with
  their count and the microseconds for one call; the calls of `RebuildFeatureTraces` apart in the same way).
- The faces and points at the end for each entity, beside IN1, PD of FP2-0 and OUT1; the count of traces at the end
  (OUT1: 733); the faces that are left above the limit, by reason (`feature_dominated`, `fold_all`,
  `all_feature_edges` as in FP2-0 (T2:341), refused, blocked, cap; a face takes the reason of the last pass in which
  it was above the limit).
- Save the result: `points.jsonl.gz` (id, coordinates, the binding: dimension, feature, u, v) and `faces.jsonl.gz`
  (the three ids, the anchor, the entity) in the case folder, with the per-face ledger as in FP2-0.

## 2. The runs

Run A: `strict_children` off. Result PA.
1. The strict comparisons of row P of the text for IN1, for the new faces of PA and for all faces of PA
   (`ContainsEntityFace`; the 4-sample counts above 0.5, 0.7, 0.8 tol and tol; `Classify` above tol; the paired
   table float against `Classify`). The `Classify` values of the unchanged IN1 faces: compute them again or take them
   from your ledger of FP2-0 (write which). Row T (the tight lists) is NOT run in this item (skip the call of
   `MeasureTight`, S:470-475).
2. For each face of PA that fails `ContainsEntityFace` (if any): write for each corner `Admissible`, and for each
   `side` of `ForFacets` the entity test, `OrientedFace`, and the float dot product of 1.5; and the coordinates.
3. P3 as in the text, with ONE change: `coverage` for the constructor and for the reservation
   (`BoundaryAtlasBytes(...)`, your `BulkReservation`) = the coverage of the IN1 state with its `traces` replaced by
   the trace map of the prototype: `{traces = the map, faces = {}}` (`BoundaryCoverage` holds nothing more,
   `CNativeAtlas3D.hpp:26-29`; the IN1 entity coverage has no faces, and the constructor refuses faces for an entity
   state, BP:257). Places: S:19, S:23, S:28 and the `traces` field of S:662; keep `BoundaryAtlasBytes({}, ...)` of
   S:21 (the formula of `Rebind`, BP:723). The sentence of the text "they stay valid because no feature edge is
   split" does not hold any more. After a refusal: the first witness as in the text. For a trace message (your witness
   search S:508-550 has no trace branch): for each map entry in key order test (1) the key is a surface edge with
   key[0] < key[1]; (2) `NormalizeFeatureTrace` gives a chain; (3) `CompareTraces(entry, normalized)` is equal, with
   the same count of pieces; (4) the end keys: `BindingParameters` + `ReferencePointKey` of the two records against
   `pieces.front().first` and `pieces.back().last` (as BP:330-342); write the first entry that fails. For the message
   of `ValidateFeatureReplacement` (BP:345) no single edge exists: write the message and the count of map entries for
   each feature chain.
4. P4 as in the text, if P3 accepted PA.

Run B: only if P3 refused PA with the message "Physical face violates its model entity/orientation." (after item 2
recorded the failing faces). The prototype again from IN1 with `strict_children` on. Result PB. Then items 1, 3 and 4
for PB (item 2 has nothing to do by construction; write the count, expected 0).

If P3 refuses with another message (a trace, a duplicate, the topology): write the first witness, do not start run B
for it, and close the item with the report. Do not repair the result by hand.

## 3. Gates of this item

1. `git diff` against the base: no change outside `UnitTests/`, your evidence folder and the documentation files of
   section 4 item 3 of `AGENTS.md`. Build without a new warning.
2. G1: the tables are identical to the reference (the hidden tag stays).
3. No G2, G3, GQ or G5 run for this item.
4. Commit and push the test code and the report ("FP2-0b").

## 4. Report

`R/night_20261115/FP2_0B/reports/FP2_0B_<build>.md`: the rows of 1.7 for each run; the strict comparisons; P3 and P4
(for each entity the two bounds, accepted or not, the seconds; the faces above tol by `Classify` with, for the first
20, the 4-sample value, `EntityFaceError(..., Magnitude())` and the coordinates); each "could not be reached /
stopped" case with its message; the peak memory; each correction of your test code (R3); each question (R13). Update
the progress file. Commit, push, verify that no owned job runs, stop and wait for the user.

## 5. Not in this item

Any production change. Rows B1, B2, G1, G2 and T of the text (done). The design of the surface core. Gates on the
measured values. Collapses, swaps or moves of surface points. The wall layers. A change of the target in
`ComputeMetric` or a size from the curvature of the wall (parked by the user).
