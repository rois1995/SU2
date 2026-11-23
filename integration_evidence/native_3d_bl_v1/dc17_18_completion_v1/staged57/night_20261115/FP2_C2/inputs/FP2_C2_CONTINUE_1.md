# FP2-C part 2, continuation 1: name the place of the lab failure; the strict bound becomes test 11 (`Bound`) of the surface collapse

Author: reviewer, 2026-11-23 (design review by Codex, two passes). For the main agent. The user sends it; it is a
user order. Copy this file with its SHA256 into `R/night_20261115/FP2_C2/inputs/`.
Code state: the checkpoint of the failed lab (`71d4ba519b`; production sources = `553ea0f147`, build652) = S0 of this
continuation. Build652 is the baseline of this continuation (also for "option off"); build647 is the historical
comparison. This text continues `FAST_PATH_STAGE2_C2.md` (SHA256 `fdd678fe...7cae`, "the order"); where the two
differ, this text wins. The watchdog rule of `FP2_C1_CONTINUE_2.md` holds: a time rule or a wall limit of a helper
is a report beside the host load, not a gate. This includes the lab condition "no pass above 600 s" of the order:
from now it is a report.
For your record: the OPEN time flag of FP2-C1 (`de08269a8a`) is ACCEPTED by the user (2026-11-23). No action.
Short names as in the order, and: `Col` = `CNativeLocalCore3DCollapse.cpp`; `Lab` =
`UnitTests/SU2_CFD/adaptation/CNativeLocalCoreLab_tests.cpp`; `BP` =
`Common/src/adaptation/CNativeBoundaryParameters3D.cpp`; `AR` = `Common/src/adaptation/CNativeAffineReference.cpp`.

## 1. Why

Your lab run N1 (build652) failed `classify_unexplained == 0` with 4 faces. Your diagnosis is correct: no wiring
error; the screens of the order are not sufficient. What is established, and what is not:
- Established: the 4 faces are children of step C (every other face is bit-equal to a face of build647, where the
  same census found 0 of 87,814). Each passed test 9 (4 points of the face: centroid and edge midpoints, against 0.7
  of the tolerance). The census asks the whole face (`EntityFaceError` with `Classify`). So 4 samples do not bound
  what the census measures.
- Established: the two printed numbers are not sizes of a gap. `model_to_mesh=1.124e-2` is the first coarse bound of
  the sheet, which the query returns when it finds one model point outside (`AR:2495`, `AR:2507`); it says only
  that one model point of sheet 10154 is certified more than the tolerance from every face of that sheet.
  `mesh_to_model=1.037e-4` is the bound of the first face above the tolerance; then the loop leaves the entity
  (`BP:556`).
- Not known: why the 4 faces are outside (a deviation of third degree that is zero at all 4 sample points is one
  explanation; a conservative refusal of the query near the tolerance is another), and where the model point of the
  model-side failure is (under one of the 4 faces, or at another place where the collapse uncovered the model
  between the witnesses of test 5).
So this continuation does two things. Step 1 makes the lab NAME the place of both failures on the unchanged
production code (test code only). Step 2 adds a test to step C that asks the function of the census for each child.
The model side gets no test in a trial in this order; the lab now checks it on its own (5.1), and its rows name
the place if it fails.

## 2. Limits

As section 2 of the order. No change of the engine, of `EntityFaceError`, of the affine reference, of `Limit()`, of
the tolerance, of tests 1 to 10 or of their order. With `surface_collapses` off, every result is that of build652
with the option off.

## 3. The change of step 2 (production)

3.1 The interface `SurfaceGeometry` gets one more pure virtual function, not const:
`bool WithinTolerance(const SurfaceFace&)` ("the strict bound of this face, as the classification of the final
proof computes it, is not above the geometry tolerance"). `StateSurfaceGeometry` (`SG`) implements it:
`impl_->BeginQuery()` (as `Chord`), then
`state.EntityFaceError(facet, bindings, points, &impl_->budget, SurfaceQueryNeed::Classify()) <= state.GeometryTolerance()`
with exactly the inputs that the rebuilt state will give to the census (`Lab:262-302`; `OwnedFaceReverseBounds`,
`BP:550-565`):
- the vertex ids rotated so that the smallest id is first, the cyclic order kept (`FaceOrder`, `SG:275`);
- `facet` = `state.Geometry().Canonical(face.facet)` (as the state build stores it, `SG:462`);
- the binding and the coordinates of each corner from the overlay (`impl_->Corners`, which reads created points
  through `At`), in the rotated order.
Comment: "The support of the query depends on the first vertex (`BP:529-536`). With these inputs the trial asks the
same query as the final classification. The answer of that query can still depend on the storage that is admitted
at the moment of the call (a workspace that is not admitted changes the traversal), so the equality is expected,
not proven; the proof of the rebuilt state stays the gate."
A storage, work or deadline failure of the query is the existing failure of the surface budget: it throws as
today; do not turn it into a refusal. The scratch of the query is charged as that of `Chord` and `Propose`: above
the caller bytes that `Run` sets (`Run:1015`) plus the resident bytes of the overlay. The arrays of the pass are
not in that baseline. This is the existing convention; this order does not change it, and it is a debt that the
report names: write the cumulative peak of the overlay (`PeakBytes()`) at each pass boundary, and beside the memory
ceiling a conservative sum for each rank and pass: (admitted bytes of the pass) + (the largest scratch increment of
one query of that pass, measured against the baseline of that same query). A sum above the ceiling is a finding in
the report, not a failure. Build652 recorded only its combined modeled peak: use that for the historical comparison
and write "not available" for its other fields. The job memory gate (19073 MiB) does not change.
The two wrappers forward the function (`RecordingSurface`, `SurfaceLongestEdgeProbe`). In the pure fakes it throws,
unless a fixture overrides it (as the three functions of 3.3 of the order).

3.2 Test 11 in `Mesh::CollapseSurface` (`Col:294-356`): `geometry.WithinTolerance(child)` for each child, in the
order of the children; the first child that is outside refuses the direction with the new reason `Bound` (a new
value of `SurfaceCollapseRefusal` after `Entity`; append it to both refusal arrays (`Core.hpp:89-92`, `142-144`)
and to the printed table of reasons; `Bound` is the last entry of the reason list of 3.4 of the order). It is
evaluated lazily with tests 9 and 10, after them, under the same rule: first for the direction with the smaller
size score; for the other direction only if the first is refused in test 9, 10 or 11. Test 9 and test 10 stay
(cheap filters before the costly query). A refused trial leaves the logical state equal (points, cells, bindings,
`frozen_`, the overlay records); the query caches of the geometry and the counters of the budget may change.
Comment at the test: "The samples of test 9 are 4 points of the face. This test asks the function of the final
classification for the whole face. The model-to-mesh side has no test in a trial (first version); the final proof
checks it."
In section 7 of the order, the sentence "The strict bound (`EntityFaceError`) in a trial: ..." no longer holds.

3.3 Print only. In the row `NATIVE3D_LOCAL_SURFACE` (`Surface:196-204`): `c_refused_bound` right after
`c_refused_entity`; `seconds_c_bound` right after `seconds_c_entity`; then `c_bound_queries` (the count of
`WithinTolerance` calls of step C).

## 4. Fixtures of step 2 (hidden tags, as the order)

Every fixture of the order that accepts a collapse with a fake geometry overrides `WithinTolerance` to true; their
results do not change. Fixture (g) of the order compares the logical state (3.2), not the caches. New:
(m) S33, fake geometry (`Samples` zero, `OnEntityFace` true), two neighbour points of dimension 2 for which BOTH
    directions pass tests 1 to 10 (for example (1, 1, 0) and (2, 1, 0), which are coplanar with their rings; set
    `shape_test = false` if `Shape` refuses; if this pair does not work, choose another pair and write it). First
    with `WithinTolerance` true: record the direction D1 that is committed. Then on a fresh mesh:
    - `WithinTolerance` false for every face: refused `Bound`; `c_refused_bound` rises by 2; the logical state is
      equal;
    - `WithinTolerance` false only for the faces that hold the kept point of D1: the other direction is committed;
      `c_refused_bound` rises by 1;
    - the call order of a direction is `Samples` of every child, `OnEntityFace` of every child, then
      `WithinTolerance`; `WithinTolerance` is not called after the first child that is outside, and not at all for
      a direction that test 9 or 10 refused (a recording fake).
(n) The real overlay on S33 (`EntityReference`): for each physical face and each of its three rotations,
    `WithinTolerance` is true and equals the comparison of the census for that face on the state; after a
    `SplitSurface` (a created point that only the overlay holds), `WithinTolerance` of each child of the split
    does not throw and is true; fixture (a) of the order with the real overlay is still accepted.
(o) A new section of fixture (i) of the order (2 ranks, its y-partition). On rank 0 split the bottom edge
    (1, 0, 0)-(1, 1, 0) to create n; then split (0, 0, 0)-(1, 1, 0) to create m; then collapse m onto (0, 0, 0)
    (the undo of the second split; n stays). A test-only forwarding geometry records the answers of
    `WithinTolerance` by sorted face key during the collapse. With the commit sink choose a child that holds n;
    broadcast its key and its recorded answer; exchange the overlay and build the state (two splits, one removal);
    the census comparison for that face on the rebuilt state is equal to the recorded answer on both ranks.
If you add standalone cases, update the case counts that `run_c2_hidden.py` expects.
The real curved case (both answers on a real mapped model) is the lab: 5.2.

## 5. The lab

5.1 Test code for BOTH steps (in `ProveLabSurface` and `ProveVolumeHandover`, serial and parallel):
- give the set of the sink faces of step C to both proof paths: the serial caller passes its local set; in the
  parallel path keep the union of the sink keys on rank 0 during the existing exchange that removes their reasons
  (today the keys are dropped after it). In this C2 mode the census uses the adjusted reasons, keeps the values
  that the rows below need, and the new condition below is on. The older callers of these helpers (FP2-A, FP2-B)
  keep their present qualification;
- the census also counts the faces above the tolerance for each entity. New condition, beside the old ones: an
  entity with no face above the tolerance in the census must not be `outside_tolerance` on the model side. Print
  `model_gap_entities=` (the count of entities that fail this) in the proof row; the trial is qualified only when
  it is 0. (Today a trial with `classify_above_tol=0` qualifies also when a sheet is outside on the model side; the
  production proof would reject that sheet.)
- print and flush the entity rows and the proof row BEFORE the rows below and before any assertion;
- for each face of the census above the tolerance with no step-D reason (at most 200 rows): one row
  `FP2_C2 LAB_ABOVE key= v_ids= v0= v1= v2= facet= body= sheet= marker= c_face=0|1 bound= direct=0|1 dims= edges=`
  (the stored tuple, the coordinates, the entity, `c_face` = the key is in the set of the sink of step C, the
  census value, `direct` = every corner binding is supported by the facet (`Geometry().Supports`), the mobility
  dimension of each corner, the three Euclidean edge lengths);
- for each entity that fails the new condition or is `outside_tolerance`: one row
  `FP2_C2 LAB_MODEL_GAP body= sheet= marker= facet= f0= f1= f2= faces_near=`. Collect the sorted, unique ids of
  the reference facets of the entity (`state.Entity(id)`) and all its physical triangles (from `state.Surface()`
  and `state.Records()`). Bisect the facet list: call
  `state.Geometry().BoundModelSurface(half, triangles, tolerance, diagnostic_budget)` (`ARh:207`), keep a half that
  is certified outside (test the second half if the first is not), down to one facet. Print its corners, and the
  keys of the faces of the SAME entity whose box is within 5e-3 of the box of that facet, each with `c_face` and
  its census value (at most 60 keys). Each call builds the face union again, so this can take some minutes. The
  diagnostic budget is what is left of the proof budget, with a cooperative deadline at the earlier of its own
  deadline and 900 s after the start of the diagnosis of this entity. On exhaustion, on an exception, or when
  neither half is certified outside: print `FP2_C2 LAB_MODEL_GAP ... failed=<reason>` and keep the rows that are
  already printed.
5.2 Test code for step 2 only:
- the probe (`VH:145-178`) counts, for the surviving faces of the sink, those with `WithinTolerance` false:
  `strict_above`. Gate: 0;
- after the final writeback and the exchange and shift of the last pass, before `BuildSurfaceState`: each rank
  asks `WithinTolerance` for each physical face that it owns (the faces that the state build reads, with its
  orientation and facet); key the answers by the sorted vertex ids; gather them with `GatherVolumeFaces`; there
  must be exactly one answer for each face of the rebuilt state. The census then prints in the proof row
  `strict_checked` (gate: equal to `state.Surface().size()`), `strict_mismatch` (the answer differs from the census
  comparison on the rebuilt state; a REPORT) and `strict_sink_mismatch` (the mismatches whose key is in the sink
  set; gate: 0), and one row `FP2_C2 LAB_MISMATCH` with the fields of `LAB_ABOVE` plus the two answers for each
  mismatch (at most 50). Count the kept and gathered answer maps in the modeled bytes of the lab, and report the
  seconds of this observation apart from the pass and the proof seconds (it can warm the caches of the proof).
- `FP2_C2/compare_lab.py`: the repeat comparison is exact for `strict_sink_mismatch`, `strict_checked`,
  `strict_above`, `c_refused_bound`, `c_bound_queries` and `model_gap_entities`; `strict_mismatch` is not compared.
  The lab runner (`run_c2_lab.py`) requires and validates the new fields (16 refusal fields, the probe fields) and
  counts a new result gate as a result failure, not as a watchdog event. Keep `baseline_build=647` in the lab
  receipt that the S launcher reads (historical comparison); record build652 apart as the baseline of this
  continuation, with its N1 data.

## 6. The order of the work

0. Before the first edit: one R4 source check of this whole continuation with what it inherits from the order.
   The watchdog rule of `FP2_C1_CONTINUE_2.md` (one repeat with three times the time, then OPEN) comes before the
   branch "failed" of every step here; the R6 limit of fixture (i) stays.
1. STEP 1 (test code only; verify the source pins of `Common` and `SU2_CFD` against `553ea0f147`): the lab changes
   of 5.1. Build the test driver. Run the lab case N1 one time with it, in a fresh evidence folder, with the pinned
   N1 inputs. The trial is expected to FAIL its proof as before, now with the rows `LAB_ABOVE` and `LAB_MODEL_GAP`:
   use a diagnostic receipt that accepts this expected failure and needs neither the fixture receipts of step 2
   nor a final cell digest (the present `run_c2_lab.py` asks for both). Required: `collapses_surface` 5,937,
   `classify_above_tol` 4, excess bins `0,2,1,0,1`. If a value differs: keep the evidence, write the difference,
   do not start step 2, stop and wait. If they are equal: keep the rows (also a `failed=` row), write them in the
   report, commit and push ("FP2-C2 continuation 1: lab rows of build652"). Do NOT wait for an answer: go on with
   item 2.
2. STEP 2: the change of section 3, the fixtures of section 4, the lab code of
   5.2. Build both targets without a new warning. All fixtures of the order and of this text; the option-off
   cohort, the 18 FP1 cohorts, G1 identical (375 / 114 / 228). Commit and push ("FP2-C2 continuation 1: fixtures
   passed; default path not yet proven").
3. The lab: section 5 of the order with 5.1 and 5.2 of this text, on the new build, all six trials (N1, N6, N6P,
   two times each). Report beside build652 (N1: 5,937 collapses in 25 sweeps, step C 25.5 s, pass 176.7 s, bad
   cells after V 3,581, `seconds_classify` 67.9): the collapses for each sweep, `c_refused_bound`,
   `c_bound_queries`, `seconds_c_bound`, `strict_checked` / `strict_mismatch`, `seconds_classify`, the pass
   seconds, the peaks of 3.1; for the 6-rank cases the values of each rank and the slowest rank.
   - Passed: commit and push ("FP2-C2: lab passed; default path not yet proven").
   - Failed: one read-only diagnosis pass (R5); one correction and one repeat only for a wiring error against this
     text. Else commit "FP2-C2: lab FAILED (<mesh side | model side | other>); default path not yet proven" with
     the rows of 5.1 and 5.2 in the report, verify that no owned job runs, stop and wait. No revert.
4. The run S and the close: items 3 and 4 of section 6 of the order, not changed (label "FP2-C2: S passed; default
   path not yet proven"; the report also has the history of the failed lab of build652).
5. Only after the label "FP2-C2: S passed": go on with FP2-D (`reviewer_handoff/design/FAST_PATH_STAGE2_D1.md`,
   SHA256 `3ed64a17664f3d144c7549a8e847fafa071649a1e16106c36dd7f02e4bd645a3`; its own text has its start
   condition). This replaces "STOP and wait for the user" of item 4 of the order for that case. In every other
   case: no revert, commit the evidence with its label, verify that no owned job runs, stop and wait.

## 7. Not in this order

A test of the model-to-mesh side in a trial (a later order, if the rows of 5.1 show that it is necessary). More
sample points in test 9. A change of step D. A change of the reverse witnesses. A change of the storage baseline of
the overlay queries (3.1 names the debt).
