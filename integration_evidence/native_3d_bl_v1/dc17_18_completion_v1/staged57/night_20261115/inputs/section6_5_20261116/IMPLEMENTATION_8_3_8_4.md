# Implementation text for design steps 8.3 and 8.4 (first part), and the state of 8.4a

Author: reviewer, 2026-11-16, for the main agent. The user accepted design revision 2
with steps 8.3, 8.4 and 8.4a on 2026-11-16. This text is the "how". The design
(`ADAPTATION_ONE_ENGINE_DESIGN_V1.md`, D) is the "why".

Rules of work: the rules of section 2 of `NIGHT_WORK_ORDER.md`. One source check for each
item before its first production edit (R4). Line numbers below are from commit
`a06d1c85e2` and are only a help; find each place by its symbol. After the cut of `Adapt`
the lambdas named here are functions of the cut units; the names of the checks stay.

Short names: Eng = `Common/src/adaptation/CNativeEngine3D.cpp`, Bnd =
`Common/src/adaptation/CNativeBoundary3D.cpp`, BP =
`Common/src/adaptation/CNativeBoundaryParameters3D.cpp`, AR =
`Common/src/adaptation/CNativeAffineReference.cpp`, T =
`UnitTests/Common/adaptation/CNativeAffineReference_tests.cpp`, tau = the geometry
tolerance of the event (`EngineOptions::surface_tolerance`).

## 0. What the measurements say (the reason for the content of this text)

Serial wall stage of the BL case (G3 reference run of 2026-11-15, case
`R/cases/n4_bl_20261115T224821_1945316`, `PrepareSurface` from the saved 492 state,
389 s, 5,402 rounds):

| Number | Value |
|---|---|
| Candidates | 6,445 |
| Trials that reach the operators (`geometry_trials`) | 3,798 |
| Refused there (`geometry_rejected`) | 2,117 (56 %); the reasons are NOT counted today |
| Trials that reach the guidance check (`guidance_trials`) | 1,681 |
| Refused by the guidance check (`guidance_rejected`) | 1,203 (72 %) |
| Accepted splits | 473 |
| Time in the guidance check (`guidance_seconds`) | 167 s of 389 s |
| Time in exact geometry queries (`upper_seconds`) | 242 s; 15,960 computed union queries, 15 ms each |
| State at the exit | `geometry_bad` 2,922, `size_bad` 5,062, census not complete |

Cause of the 72 % (C, `GeometryGuidanceExcess`, Bnd:3466-3529): the check adds
`max(0, error - tau)^2` over the faces of the patch, with no weight for the area. A split
that cuts one failing face into two failing faces makes this sum larger, also when no
face became worse. Then `geometric_change` (Eng:4174-4214) rejects the split. This is the
progress veto that design section 2 removes ("an edit is applied when it is needed and
valid").

No-BL full run (clean N5 run, rank 0, event 1): `guidance_rejected` 398 of 6,080 (6.5 %);
`SmoothBoundary` guidance refusals about 762 of 6,890 (11 %); about half of the exact
queries are answered by the memo. So in the quality stage the exact check for a trial
nearly always confirms the trial. See part C.

## Part A. Step 8.3 - identical results

Change against D: step 8.3 puts the geometry law into the code as a SERVICE that stages
call, not into the composed size target (D section 3, R2.1). Reasons (source inventory):
a law in the size target would split wall faces that already pass the exact proof (it
marks 9.1 % of the edges of a good wall), and a non-empty composition switches batching,
collapse planning and `track_splits` for the no-BL case (Eng:1368, 1486, 3668-3678). So
8.3 changes no mesh. The curvature cap of the BL metric (`CBoundaryLayerMetric.cpp`
2393-2423) is not changed in this step.

### A1. The wall geometry law as production code

1. New files `Common/include/adaptation/CNativeWallGeometryLaw3D.hpp` and
   `Common/src/adaptation/CNativeWallGeometryLaw3D.cpp` (add to the build list beside
   the other adaptation sources).
2. MOVE the logic of the N5 diagnostic from the test file into these files. Do not keep
   a second copy in the test; the test calls the production functions.
   - `D9EndpointNeedScore` (T:2445-2454): for one end point with surface values
     (normal n, principal curvatures k1, k2, directions d1, d2) and the chord vector c
     (b - a, not unit): `max( (|k1| (d1.c)^2 + |k2| (d2.c)^2) / 8 , |n.c| / 4 )`.
   - The eligible sides of an end point (T:2429-2444) and the maximum over them
     (`D9ScoreEndpoint`, T:2466-2495): same marker and same `Entity` as the anchor facet.
   - The edge value (`D9EvaluateEdges`, T:2496-2533): the maximum of the two end points.
3. Interface (names in words; the exact argument types follow the moved code):
   - `WallEdgeEstimate EstimateWallEdge(state, node a, node b, anchor facet)` gives
     `{supported, value}`. `supported` is false when an end point has no record, no
     eligible side, `EvaluateSurface` throws, or a value is not finite. A wall edge with
     two wall faces: the caller takes the larger value of the two anchor facets.
   - `bool EstimateSaysNeed(estimate, tau)` = `supported && value > 0.75 * tau`.
     tau is an argument. The constant `D9Tau` of the test goes away.
4. Pure functions. No cache, no static state, no counter inside, no lock (D 5.8).
5. Print-only census, only with `options.timings` (as the D9 counters): at the entry and
   at `finish` of each `Adapt` call with entity geometry, each rank loops over its owned
   wall faces (`physical[i].present`) and the three edges of each face (anchor = the
   facet of the face) and prints ONE rank-local line:
   `NATIVE3D_WALL_LAW rank= boundary=entry|exit faces= edge_incidences=
   need_incidences= unsupported_incidences= max_ratio= seconds=`
   (`max_ratio` = largest value / (0.75 tau)). No collective call. The time is inside a
   named timer operation `wall_law`, so the 95 % coverage gate holds.

Checks of A1:
- The three N5 geometry fixtures give the same M1 numbers and the same edge CSV hashes
  as their R1-repeat receipts (7,521 / 6,759 / 4,368 need edges).
- New unit cases: a planar side gives 0; the value is the same for (a, b) and (b, a);
  an end point with no eligible side gives `supported = false`.

### A2. The requirements record, as a read-only view

1. New header `Common/include/adaptation/CNativeRequirements3D.hpp`:
   `struct AdaptationRequirements` with five parts, filled from data that exists:
   - size: `hmin, hmax, ARmax, hgrad, complexity` (metric family controls) and
     `composition_present`;
   - geometry: `mapped`, `entity_geometry`, `curved` (extension components not empty),
     `tolerance`;
   - features: `policy_present`, the counts of features, corners and sheets, `angle`;
   - wall rows: for each marker `first_height, growth, thickness`, and the height
     tolerance (from `wall_heights` and `region_walls`); empty with no BL;
   - budgets: `maximum_work, maximum_seconds, memory_ceiling, job_memory_limit`.
2. One function builds it at the top of `Adapt` (after the cut: in the setup unit). It
   reads; it changes nothing. Each rank prints one line `NATIVE3D_REQUIREMENTS ...` with
   `options.timings`.
3. In this step NO existing test in the code is replaced by the record.
4. Report (no code): the table of the places in Eng that test `composition`,
   `surface_target`, `wall_heights.empty()`, `region_walls` or `curved` (128 lines by
   the reviewer's count, 119 inside `Adapt`). For each place: symbol, the test, and one
   class: (R) it is a check of a requirement, (P) it is a rule on a property that can be
   measured, (X) it selects a code path from the kind of the case.

### A3. One measurement with part A: M6b

Unit fixture on the accepted mesh (the family of the N5 geometry fixtures): for each wall
edge, project the chord midpoint with `BindNewPoint`, and give the exact `Orientation` of
the projected point against each old wall face of the edge shell. Counts of positive,
zero and negative for all 82,559 edges. No split is done.

### Gates of part A

Identical results. G1 by identity against its reference (cut plan 2.1), G2, G3, GQ with
ALL old integer rows equal (the new rows are additions), G4, and ONE full two-event run
whose six files are equal to the references of that time. Report the census numbers and
the seconds of `wall_law`.

## Part B. Step 8.4, first part: the wall stage accepts each needed and valid split

Scope: the wall stage of today (`PrepareSurface`, the `Adapt` call with a surface
target). The no-BL workflow must NOT change: `geometric_change` is already skipped for
its construction splits (Eng:4177). Each change of part B has the condition "this
`Adapt` call has a surface target".

### B0. Measurement first (print only, identical results)

Add counters, print them at the stage exit, and run the G3 fixture (C492, serial,
unchanged 388.8 s budget). No decision in the code reads them.

1. The D9 site counters also count in the timer phase `surface` (today only `quality`
   and `final_proof`), so that the queries of call sites (a) and (e) of the wall stage
   are known.
2. For each trial that `guarded_geometry` (Eng:4160-4166) refuses: one class. The
   classes: `SplitEdge` refused and `InsertEdgeCavity` refused (each with the class of
   its `reason` text: orientation, interface, surface containment, feature, marker
   junction, incomplete star, other); `PreservePassingFirstHeights`; `height_check`;
   `boundary_replacement_ready`. Line: `NATIVE3D_WALL_REFUSAL class= count=`.
3. For `geometric_change`: the count of rejections by cause (result not complete; a
   geometric choice with no progress; after above before) and by the origin of the
   choice (geometry queue or size queue).
4. For the candidates that do not reach a trial (6,445 - 3,798 in the reference): the
   count for each disposition.
5. Rejections after the guidance check: by `ValidateMetricChange`, by `metric_trial`.

Report one table. The sums must agree with the counters that exist (`geometry_trials`,
`geometry_rejected`, `guidance_trials`, `guidance_rejected`, `accepted`).
Gate: G3 identity (the first 336 edits, all old integer rows), G2.

### B1. No progress veto for a needed wall split

Rule (D section 2): an edit is applied when it is needed and valid.

1. In the trial of an edge SPLIT, when the `Adapt` call has a surface target, and the
   edge comes from the size queue or from the geometry queue of the wall stage, and the
   trial is not a coarsening and not a repair: `geometric_change` is not called. The
   trial continues as if it passed, with "no geometric progress known" for
   `metric_trial` (its progress veto is for the kind `GeometryRepair` only,
   Eng:1629-1633; verify this in the source check).
2. All validity stays: the operators with their exact checks (positive cells,
   `ValidatePrescribedInterface`, `CheckSurface`, `ContainsEntityFace`, feature traces),
   `PreservePassingFirstHeights`, `height_check`, the body separation at the commit.
3. The joint path (`place_patch`, the expansion, the growth loop) is not changed. It
   keeps its `validate_private` calls with the guidance check, because its edits are
   repairs of a refused split, not needed-and-valid splits.
4. The new faces of an accepted split are measured exactly when their cells get their
   census rows (`Selection::Add`, call site (e)), as today. A new face that fails stays
   in the geometry queue. So each wall face still gets its exact proof, one time, and
   the final gate does not change.
5. New counter: `wall_splits_without_guidance`.

Why the stage still ends: each new wall point is on the surface, a split makes faces
smaller, and a failing face stays in the queue until it passes (D 4.1, R2).

Expected on the G3 fixture (I, to verify): the 167 s of the guidance check go to zero for
these splits, and about 1,200 more trials reach the metric checks in the same budget.

### B2. Inside a failing face, the law chooses the edge

Reason: with B1 the veto no longer removes a split of the wrong edge. So the choice must
be right. Today each edge of a failing face gets the score
`(error - tau)^2 * |e|^2` (Eng:936-963), which prefers the longest edge, also when the
curvature is along another edge.

1. Only when the `Adapt` call has a surface target: for a failing face, compute the
   estimate s(e) of its three edges with A1 (anchor = the facet of the face). Let
   s_max be the largest. The score that the edge gets from this face is the score of
   today times `(s(e) / s_max)^2`. When s_max is 0 or one estimate is not supported, the
   factor is 1 for the three edges (the score of today).
2. Nothing else in `Selection::Add` changes (debts, flags, bins).
3. New counters: `law_weighted_faces`, `law_fallback_faces`.

Do B1 and B2 as two sub-steps, each with its own run and its own numbers.

### Gates of B1 and B2

1. No-BL: identical. G2 and GQ with all old integer rows equal; ONE full two-event run
   with the six files equal to the references of that time. If a file differs, the
   condition "surface target" leaks: R5.
2. Unit tests: G1 by identity, except the cases that test the wall stage; list each
   changed case with the old and the new numbers.
3. BL wall stage, G3 fixture (serial, unchanged budget and inputs), run TWO times:
   - validity: all cells positive, no validity failure, no new refusal class;
   - the two runs have the same edits up to the smaller of their accepted counts;
   - purpose of the item: at the exit, `geometry_bad + size_bad` is below the reference
     7,984, or the stage ends with the surface ready. If not: the item did not do what
     it is for; preserve the numbers, revert it (R5, cause in the proposal).
   Report beside the reference: accepted splits, rounds, `geometry_trials`,
   `geometry_rejected`, `guidance_trials`, `guidance_rejected`, `guidance_seconds`,
   `upper_seconds`, measured cells, `geometry_bad`, `size_bad`, the B0 tables.
   The new G3 rows are the reference for later steps.
4. The same wall stage on 6 ranks from the saved state, if the fixture permits it (D 4.4
   says it does): report the same numbers. It is a report, not a gate of B1 / B2.
5. The standing safeguards for a change of shared code (481, current target, 2D at
   1 / 2 / 4 ranks).

### What part B does not do yet (the reviewer writes it after the B0 and B1 numbers)

- The 2,117 refusals at the operators (56 % of the trials). Their cause is not known
  until B0 item 2. No fix is written before that.
- The wall stage on all ranks inside the production BL pipeline (D 4.4; needs step 8.2c).
- The stored proof of a face that travels with the cell (D 4.3). The source check of
  the reviewer shows: the census row has no value for a face and is not in the
  checkpoint; the natural place is `OwnedCell::physical` (`FacetBinding`,
  `CNativeDistributed3D.hpp:15-31`). The query memo that exists already answers a
  repeated query (50 to 67 % hits), so the gain of a stored proof must be measured
  first (B0 item 1).
- The cone operator with a projected point: `InsertEdgeCavity` does not call
  `BindNewPoint` and refuses an edge between two canonical facets (Bnd:1607-1676).

## Part C. Step 8.4a - not to implement now

The user accepted 8.4a. The reviewer's reading of the numbers afterwards is: its gain is
small, and it changes an acceptance rule. So it is NOT in the work order now.

- The exact queries of the quality stage cost about 245 s for each rank in event 1
  (11 % of the event). Call sites: (a) about 36 %, (b) about 55 %, (e) about 9 % of the
  computed queries (rank 0; the memo already answers 67 %, 34 % and 83 % of the calls).
- The exact check of a trial rejects only 6.5 % of the trials at (a) and about 11 % at
  (b). A trial that is accepted needs the exact proof of its new faces anyway (one time,
  at the commit). So an estimate in the place of the exact check can save only the
  queries of the rejected trials: about 10 % of 245 s, about 25 s for each rank.
- The large costs of the quality stage are the communication (about 290 s) and the join
  wait (about 540 s). They are the subject of step 8.5 (passes).

The reviewer tells this to the user. The decision to drop or to keep 8.4a is the user's.
