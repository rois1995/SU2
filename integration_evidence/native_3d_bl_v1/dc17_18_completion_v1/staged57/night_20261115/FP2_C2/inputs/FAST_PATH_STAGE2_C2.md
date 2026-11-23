# Fast path stage 2, item FP2-C part 2: the core removes sheet points of the surface (first version: "inside" rule)

Author: reviewer, 2026-11-23 (checked against the source; second design review by Codex). For the main agent. The
user sends it; it is a user order. Copy this file with its SHA256 into `R/night_20261115/FP2_C2/inputs/`.
Code state: start only after FP2-C part 1 has one of the two labels of item 4 of its continuation 1 ("FP2-C1:
accepted (S = S2)" or "FP2-C1: accepted, time rule OPEN for the user"). S0 = that commit (production sources =
`4730bd59ec`; `UnitTests` = `4730bd59ec`). If part 1 was reverted, its default proof or S2 failed, or it has no
label: do not start; write it; stop. Write S0 and its build in the progress file. You may take test code from commit
`ebf5bfff8f` (the dry measurement of step 0).
Short names: `Core.hpp` = `Common/include/adaptation/CNativeLocalCore3D.hpp`; `Surf.hpp` =
`CNativeLocalSurface3D.hpp`; `Mesh`, `Changes`, `Pass`, `Surface`, `Measures` = `CNativeLocalCore3D<name>.cpp`;
`SG` = `CNativeLocalSurfaceGeometry3D.cpp`; `Run` = `NativeRemeshRun.inc`; `VH` =
`UnitTests/SU2_CFD/adaptation/fixtures/native_volume_handover.inc`. Line numbers are those of `c843610249`; on S0
they are equal except `Core.hpp:276-280` -> `282-286` and `Changes:206-219` -> `210-223`.

## 1. Why

The surface of the hand-over has too many points for the target: 90 % of its edges are short. Only the engine removes
surface points today (about 3,100 in each event), in its slow collapse pass. The core must do this work before the
engine can leave that pass out. The dry measurement of step 0 (your report `FP2_C2_0_648.md`) gave the numbers for
the design:
- about 66,000 to 71,000 directions pass all tests; the float samples and the strict bound agree (1 exception);
- but 89 % of them move the surface OUTWARD (out of the old cells). On a thin body part (a sharp trailing edge) an
  outward step can go through the other side of the body, and no present test of the core sees that. The engine has a
  contact test for this; the core has none.
So this first version accepts only collapses that are INSIDE: no new face is outside the old cells. For these there
is a proof that no contact is possible (3.4 test 4). The step-0 estimate for this rule is about 1,900 to 2,300 points
in one sweep. The outward collapses come in a later item, with a contact test.

## 2. Limits of the item

Production change in the local core and its surface geometry only. No change of the engine, of `Limit()`, of the
tolerance, of the metric, of `CollapseNear`, `Collapse`, `Split`, the V rounds or the structs of `BytesPerCell`.
`SplitSurface` and the split path of `Prepare` give the same results as today. With the new option off (3.1) every
run and every fixture gives the results of S0, with one exception that is not a result: the new members of the
overlay raise its modeled bytes by a constant. The default path (both config options NO) and the run CORE do not
reach the new code. Readable code (D 7.1a): one function for each test group; a comment with the reason at each
test. `CollapseSurface` and its helpers go in a new file `CNativeLocalCore3DCollapse.cpp` (add it to
`Common/src/meson.build`).

## 3. The change

3.1 Switch. `CoreOptions::surface_collapses` (default false; the last field of the struct, after `wall_collapses`).
`SurfaceWork` runs step C (3.6) only when it is set. `Run` sets it together with `surface_splits` (where
`wall_collapses` is set, `Run:1018` on S0); the lab sets it too (`VH`, where it sets `wall_collapses`).

3.2 Notation. E = (p, k) is an edge of the surface; p is removed, k is kept and does not move. Star(p) = the cells at
p; Ring = the cells that hold p and k. E is on two physical faces (p, k, c1) and (p, k, c2). A CHANGED face is a
physical face (p, a, b) at p without k. Its CHILD is that face with p replaced by k in place (the orientation stays),
with the binding of the parent. The old cell of a changed face is X = (p, a, b, c); its new cell is N = X with p
replaced by k. A SPOKE is an edge (p, x) of a physical face at p. A DIAGONAL is an edge (k, y) of a child that is
not an edge of a physical face before the collapse (a new edge, or a volume edge that becomes a surface edge); after
the skin condition of test 2 these are exactly the child edges (k, y) with y not c1 or c2.

3.3 The interface `SurfaceGeometry` (`Surf.hpp:39-50`) gets three pure virtual functions:
`unsigned Dimension(Id) const` (`state.Geometry().MobilityDimension(At(id).binding, state.Mobility())`, as `SG:616`),
`void Remove(Id)` (3.7) and `bool OnEntityFace(const SurfaceFace&) const` (the test of the existing `OnEntity` for a
face of three existing records). The two wrappers forward them (`RecordingSurface`, `SurfaceLongestEdgeProbe`). In
the pure fakes (`FlatGeometry` and its children) they throw, unless a new fixture overrides them.

3.4 `Mesh::CollapseSurface(Edge edge, Slot seed, SurfaceGeometry& geometry, uint32_t stamp, SurfaceCounters&
counters, bool report, const std::function<void(const SurfaceFace&)>& sink = {})` (public, as `SplitSurface`). It
returns a new enum `SurfaceCollapseRefusal` (`Core.hpp`; `None` and one value for each reason below), not
`SurfaceRefusal`: the split counters, `surface_refusal_sink` and `RefusalCount` do not see it. It does not test
`Length` (the candidate list of 3.6 does). It evaluates both directions (remove `edge[0]` and keep `edge[1]`, and the
reverse). The tests of a direction, in this order; the first failure is the reason of that direction and nothing is
changed. The tests are ordered by cost; the acceptance does not depend on the order.
1. Eligibility of the removed end p: `Protected` (its `Dimension` is not 2: a feature point or a corner is never
   removed in this item); `Border` (`points_[p].border`); `Ineligible` (`SurfaceParents(E, Ring).size() != 2`);
   `BorderKeeper` (k is a border point: its star is not complete on this rank, so the link test is not complete;
   first version); `FeatureChord` (a diagonal whose two ends have `Dimension` below 2: such a chord has no trace).
2. The link condition (`Link`): Lk(p) ∩ Lk(k) = Lk(pk) as complexes, in the volume and on the physical skin. With
   N(x) = the points of the cells of Star(x) without x:
   - points: N(p) ∩ N(k) equals the points of the Ring cells without p and k;
   - edges: each pair (x, y), x and y not p or k, such that a cell holds p, x, y and a cell holds k, x, y, is an
     edge of a Ring cell;
   - triangles: no cell (p, x, y, z) exists together with a cell (k, x, y, z);
   - skin points: the points that are in a physical face with p and in a physical face with k are exactly c1, c2;
   - skin edges: no pair (x, y) has the physical faces (p, x, y) and (k, x, y);
   - faces: no changed face is a face of a Ring cell (its child would be the face of that cell opposite p, which
     exists already).
   Reason in a comment: without it the collapse makes one edge, face or cell two times (Dey et al., "Topology
   preserving edge contraction"). p and k are not border points, so their stars and links are complete on this rank.
3. Rule 2 for each child: its smallest altitude in space is not below `Limit()` (`Near`; as `SplitSurface`,
   `Surface:353-367`). Move `SmallestAltitude` (`Surface:143-147`, anonymous namespace) to a declaration that both
   files see (a private static of `Mesh`, or an internal header beside the sources); `SplitSurface` stays
   bit-identical.
4. The inside rule (`Outward`): for each child (its old cell X holds no k after test 2), with its new cell
   N = (k, a, b, c): the cell N with c replaced by p is NOT exactly positive (`Positive`, the exact test; a
   zero-volume cell is not positive). In words: p is not on the cell side of the plane of any child; p on the plane
   is permitted. Comment with the proof: "The mesh at the entry of step C is valid (embedded, conforming). The cap
   tetrahedra (p, child) are then all oriented the same way as the new cells (not negative), and the new cells plus
   the caps have the same boundary as the old cavity Star(p). By degree theory each point of the cavity is in exactly
   one of them: the new cells fill the old cavity without the caps. They do not overlap (move p along the line to
   k: the cell volumes change linearly and no cap changes its sign, so no cell turns over on the way), and with the
   face matching of `Prepare` they meet each other and the kept cells only in shared faces. So they cannot touch a
   cell or a face outside the cavity: no contact test is necessary. An outward collapse needs one; it is not in
   this version."
5. The reverse distance (`Reverse`): each WITNESS point of the old faces has a distance in space to the nearest
   child (`SU2NativeCurved::detail::ClosestOnTriangle` over all children) not above `Limit()`. The witnesses: p; the
   midpoint of each spoke; the centroid of each physical face at p (also the two faces on E); and for each spoke and
   each diagonal that share no end: the closest points of the two lines through them, when the lines are not
   parallel and both parameters are strictly between 0 and 1; the witness is the point on the spoke. Comment: "A
   collapse removes a point of the model; the samples of the new faces do not see what the new faces no longer
   reach. Where the two fans are graphs over one plane, their height difference is linear on each piece of the common
   refinement, so its largest value is at p, at a rim point, or where an old spoke crosses a new diagonal. The
   distance to the nearest child is close to it, not equal (it can peak where the nearest child changes). This is a
   screen, not a proof; the final proof of the mesh stays."
6. `Prepare(Star(p), fresh, edits)` with the list of replaced faces of 3.5: `Topology`, or `Validity` for a new cell
   that is not positive in floating point. The new cells are those of `CollapsePlan` (the cells with k go away; the
   others get k in the place of p). Also `Topology`: a point of the Ring cells other than p and k that is in no new
   cell and whose star lies inside Star(p) (it would lose its last cell; `Commit` would remove it silently).
7. The size test (`Size`): the size score of `CollapsePlan` (`Changes:206-213`) is not above `LongEdge` (the longest
   new edge; it is also the score of the choice), AND no edge of a child is above `LongEdge` (a child must not
   inherit a long rim edge or turn a long volume edge into a surface edge). The shape test (`Shape`) of
   `CollapsePlan` (`Changes:215-219`). Factor the score and the shape test into one helper that both use;
   `CollapsePlan` stays bit-identical. (`CollapsePlan` itself cannot be called: `CanCollapse` is false for a surface
   point.)
8. Exact positivity of each new cell (`Validity`).
9. `geometry.Samples(child)` for each child: each sample is finite and the largest is not above `Limit()`
   (`Distance`): the same function and limit as step D.
10. `geometry.OnEntityFace(child)` for each child (`Entity`): the strict test that the state build makes.
Evaluate tests 9 and 10 lazily: first for the direction with the smaller size score (equal: the one that removes the
larger id), for the other only if the first is refused; when only one direction passes tests 1 to 8, for that
direction only. When both directions pass: take the one with the smaller size score (equal: remove the larger id).
Each refused direction adds 1 to its `c_refused_<reason>`; a direction whose tests 9 and 10 were not evaluated is
neither counted nor compared. Return `None` after a commit; else the reason of the direction whose first failure
comes later in this list (equal: the direction that removes the larger id): `Protected`, `Border`, `Ineligible`,
`BorderKeeper`, `FeatureChord`, `Link`, `Near`, `Outward`, `Reverse`, `Topology` / `Validity` of test 6, `Size`,
`Shape`, `Validity` of test 8, `Distance`, `Entity`.
Commit: `RememberWorkPoints`; `Commit`; ERASE the entry of p from `frozen_` (else `Check` throws, and a later split
can use the free slot again); `geometry.Remove(id of p)`. Under `report`, when `sink` is set: call it one time for
each child (the ids in the oriented order of its new cell, rotated so that k is first; the facet of its binding).
Step C passes `stats.surface_collapse_sink` (a new member of `PassStats`,
`std::function<void(const SurfaceFace&)>`, print-only; `Run` does not set it).
What this rule gives, in a comment at the function: "At the same metric and the same sample inputs, step C makes no
edge that step L marks and no face that the samples of step D mark. A conformity split, a feature demand or a
changed metric can still split the patch again."

3.5 `Prepare` (`Mesh:165-260`): generalise the list of replaced faces. Reading of one entry
`ReplacedFace{cell, face, a, b, m}` (`Core.hpp:276-280`): for each end of {a, b} that is not `NoSlot`, the child is
the parent with that end replaced by m; `NoSlot` gives no child for that end; a parent that holds m already gives no
child. The split passes (a, b, m) as today; the existing check `edit.a == edit.b` stays. The collapse passes
(p, `NoSlot`, k) for EACH physical face of Star(p) that holds p: the two faces on E go away with their cells; each
other one gets its child with `{Physical, 0, the binding of the parent, FaceSign(child)}`. A child key that exists
already: `Topology`. `Blocked` is not tested when the list is not empty (as the split). Every oriented outer face
that is not in the list keeps its match and its binding, as today.

3.6 Step C in `SurfaceWork` (`Surface:810-838`), after step D and before `MarkUnresolved`, only with the option of
3.1. Candidates: the edges of physical faces (`EdgeRing(edge, cell).surface`) with `Length` below
`options_.short_edge` (0.9) and at least one end with `Dimension` 2 that is not a border point. Order: `Length`
ascending, then the id pair. A candidate with a removed end is skipped (as `Pass:174`); a dead or changed seed cell
defers it (as `Surface:517-522`). Deferral: by the stamp of the sweep on the star of each end that can be removed.
The work list as step L (`NextEdges` + `RememberWorkPoints`). A memory of refusals local to step C, keyed by the id
pair with `change_`, used only under `options_.memory` (as `Pass:188-201`), freed with the step. At most
`SurfaceCollapseLimit = 50` sweeps (a named constant beside `Surface:20`); a sweep with no accepted collapse ends the
step. Step C prints no `NATIVE3D_LOCAL_SWEEP` row. `faces_out` / `points_out` of the row are counted after step C.
The tables of step C stay inside the bound of `SurfaceArrayBytes` (the tables of step D are freed before it; say it
in a comment; if you need another kind of table, extend the bound and write it in the report).

3.7 The overlay (`StateSurfaceGeometry`, `SG`). `Remove(id)`, three cases: (1) the id is in `pending_points` (made
by this rank since its last `Exchange`): erase it from `pending_points` and `created`, and erase its keys from
`supports` and `coordinates` (no other rank has them); nothing is sent. (2) the id is in `created` from an earlier
pass (made by any rank): erase it from `created`, append it to `pending_removed`; its keys stay on every rank.
(3) else the id must be a record of the state (else throw): append it to `pending_removed`. In each case insert it
into a set `removed` and `++local_removals`; a second `Remove` of an id throws. Ids are never used again (production:
`IdSequence`); `Accept` and the merge of `Exchange` throw for an id in `removed`. `At` throws for an id in `removed`;
`Find` returns nullptr for it. `Exchange` sends the rows
`RemovedRecord{id, source}` of `pending_removed` to all ranks (also an empty list: the call is collective), after
the created points. Rows whose source is this rank are skipped (already applied). For each other row: an id already
in `removed`, or one that is neither in `created` nor a state record, is a failure; else erase it from `created` if
it is there and insert it into `removed`. Then clear `pending_removed`. (Serial: `Exchange` returns at once and
`pending_points` is never cleared, so case 1 covers every created point.) Add the bytes of the removed lists to the
two admission bounds of `Exchange`, and `removed` / `pending_removed` to `ResidentBytes`.
`StateSurfaceGeometry::Removals()` (as `Splits()`); `SurfaceStateResult::removals` (the collective sum, set as
`splits`). `BuildSurfaceState` builds when the collective sum of splits PLUS removals is not 0 (today: splits
only). No trace changes (a removed point has dimension 2).

3.8 Print only. In the row `NATIVE3D_LOCAL_SURFACE`, after `seconds_v` and before the optional quoted fields:
`collapses_surface`, `c_sweeps`, `c_candidates`, `c_deferred`, one `c_refused_<reason>` for each reason of 3.4,
`c_reverse_max` (the largest witness distance of a committed collapse), `seconds_c`, `seconds_c_samples`,
`seconds_c_entity`, `faces_before_c`, `points_before_c` (counted after step D). In the row
`NATIVE3D_LOCAL_SURFACE_STATE`: `removals_all` right after `splits_all`. Step C does not touch the split fields.

## 4. Fixtures (hidden tags `[.][NativeFP2C2]`, so that the case table of G1 stays identical)

Serial where not said; `BoundMesh`, `Snapshot`, TwoCubes / `EntityReference` as the surface fixtures of FP2-A and
FP2-B. The mesh S33: the `TwoCubes()` construction extended to 3 x 3 x 1 unit cubes (the same six cells for each
cube), metric 0.25 (`BoundMesh(..., 0.25)`: metric lengths are half the Euclidean ones), with the real overlay built
as `EntityReference` (replace its two `REQUIRE`s on the TwoCubes crease ids by: every box edge is covered by
traces). Its points (1, 1, 0), (2, 1, 0), (1, 2, 0), (2, 2, 0) (and the four at z = 1) have dimension 2; all others
less.
(a) S33: `SplitSurface` of the bottom diagonal (0, 0, 0)-(1, 1, 0) gives m; `CollapseSurface` of (m, (0, 0, 0)) is
    accepted (the restored diagonal ends at the sheet point (1, 1, 0); p is on the plane of each child: permitted);
    the physical faces (oriented triples, bindings) and the cells (vertex order) equal those before the split;
    `Find(m)` is null and `Splits()` = `Removals()` = 1; `Check` passes.
(b) `CollapseSurface` called directly on (m, a), with a fake `Dimension` that gives 2 to every point except a: for a
    step-L split point m: refused `Size`. For a step-D split point m on the cylinder fixture: refused `Reverse`
    (the split point is 1 - cos(pi/8) = 0.076 from the restored parents, above 0.07; the cells are inside, so
    `Outward` cannot fire). The mesh is bit-equal.
(c) TwoCubes: a feature point and a corner are refused (`Protected`), with edges (feature, corner) and (feature,
    feature). S33: the split point m of the bottom edge (1, 0, 0)-(1, 1, 0) (a = (1, 0, 0) is a feature point)
    collapses onto a; `FindTrace` of every trace edge is unchanged.
(d) S33, 2 ranks, rank 0 owns the cubes with x-index 0: (1, 1, 0) (dimension 2, on the rank border) is refused
    `Border`. On rank 0, split the bottom diagonal (0, 0, 0)-(1, 1, 0) to m; (m, (1, 1, 0)) is refused
    `BorderKeeper`.
(e) The inside rule: S33, fake geometry (tolerance 0.02, so `Limit()` = 0.014; `Dimension` 0 for k and 2 for every
    other point; `Samples` zero; `OnEntityFace` true; `shape_test = false`): k = (2, 1, 0), p = (1, 1, 0) moved to
    (1, 1, h). h = +0.005 (toward the cells): `Outward`. h = -0.01: accepted. h = -0.02: `Reverse`.
(f) The saddle of the design review: k = (0, 0, 0), a = (1, 0, H), b = (1, 1, 0), c = (0, 1, H),
    p = (0.2, 0.02, 0.2 H), H = 3.5e-4, and the apex q = (0.5, 0.5, -1); cells (p, k, a, q), (p, a, b, q),
    (p, b, c, q), (p, c, k, q) (`BoundMesh`; every outer face physical). Fake geometry: tolerance 1e-4 (limit
    0.7e-4); `Dimension` 0 for k and 2 for every other point. The collapse p -> k passes `Near` and `Outward`, its
    witnesses p, the spoke midpoints and the face centroids are within the limit, and it is refused `Reverse` by
    the witness of the spoke (p, c) and the diagonal (k, b) (about 1.13e-4); the other direction is `Protected`.
(g) A refused trial leaves the points, the cells, the bindings and `frozen_` bit-equal, and the overlay equal
    (`Find` of each point, `Splits()`, `Removals()` and `ResidentBytes()`).
(h) A collapse and then a split that uses the free slot: `Check` passes.
(i) S33, 2 ranks: a point created in pass 1 on one rank and removed in pass 2 on the other (after a shift, as the
    FP2-A exchange fixture) is in no `created` after the exchange; a point created and removed in one pass is not
    sent, and the same placement made later by the other rank is accepted by the exchange; the state build accepts
    and the bytes of `Snapshot()` are equal on all ranks. Removals only: rank 0 owns the six cubes with y-index 0 or
    1, rank 1 the other three; rank 0 removes (1, 1, 0) onto (2, 1, 0) (no split on any rank; set
    `shape_test = false` if `Shape` refuses and write it); the collective state build accepts and the bytes of
    `Snapshot()` are equal.
(j) Work list on = work list off (the same cells and faces).
(k) The check of the proof (test code), in each accepted collapse of (a), (c), (e), (i): the volume of the new cells
    plus the volume of the cap tetrahedra equals the volume of the old cavity (relative 1e-10); each new cell is
    exactly positive; and no new cell overlaps a live cell of the fixture mesh with which it shares no point (brute
    force; a separating plane among the face planes of the two cells and the cross products of their edge pairs,
    with a margin of 1e-12 times the cell size; touching counts as separate). Pairs that share a point are covered
    by the volume identity and the exact positivity.
(l) With the option off: all surface fixtures of FP2-A, FP2-B and FP2-C part 1 give the same results as on S0.
Fixture (i) is the largest. If it reaches the limit of R6: write its state, finish the other fixtures, the cohort
and G1, commit the evidence with "FP2-C2: fixture (i) OPEN at the R6 limit; default path not yet proven", update the
progress file, verify that no owned job runs, STOP and wait (the lab needs the exchange).

## 5. The lab (after the fixtures; BEFORE a full run)

Runs N1, N6 and N6P with the production `RunPass` and the options of the lab of part 1 plus
`surface_collapses = true`, each TWO times. Reference: the lab of part 1 on build647 (N1 / N6 / N6P: final surface
faces 87,814 / 87,796 / 86,210; owned cells 475,815 / 470,940 / 468,571; bad cells after V 4,481 / 4,155 / 4,072).
Conditions:
- each cell exactly positive; `Check`; the donor scan 0; the state build accepts and (6 ranks) the bytes of
  `Snapshot()` are equal;
- the proof condition of FP2-B (each entity accepted, or each face with `Classify` above the tolerance is a face
  that step D of the last pass left with a reason) and `classify_unexplained=0`. A face that the sink of step C
  reported counts as a face WITHOUT a reason, also when the lab's map has a step-D reason for its key. So a step-C
  face above the tolerance fails the gate by design. (Your step-0 data: of the inside directions that pass all
  tests, the strict bound refused 0 of 7,034 / 8,108 / 8,193.) `classify_above_tol` is a report beside build647;
- the probe (test code): the lab collects the faces of `surface_collapse_sink`. At the end of each pass, for each
  collected face that is a physical face of the final mesh: the largest of `Samples` is not above `Limit()`; the
  smallest altitude is not below `Limit()`; `OnEntityFace(face)` accepts; each of its three edges has `Length` not
  above `LongEdge`. Violations: 0;
- no core metric edge below 1e-2 at the end; `star_max` not above 1,000; `sweeps_distance` below 40; `c_sweeps`
  of `NATIVE3D_LOCAL_SURFACE` below 50; `v_limit=0`; no pass above 600 s;
- the two runs of each case are equal: the rows `FP2_C cells` and every integer field of the rows of part 1
  section 4 (with the new fields of 3.8). Only this repeat comparison is a gate: a copy of `FP2_C1/compare_lab.py`
  in `R/night_20261115/FP2_C2/` compares the two runs of each case. Its comparison with S0 (pass-1 rows, schemas)
  and the rule of part 1 "the surface must not change in pass 1" do not apply to this item.
Report beside build647, for each case, pass and rank: the surface faces and points before and after step C; the
accepted collapses for each sweep; the refusals by reason; `c_reverse_max`; the seconds of step C (all, samples,
entity); the bad cells by class (a rise is a finding, not a failure); the cells; the modeled peak. And for N1 and
N6P: the count beside the step-0 estimate for this rule (one sweep: N1 1,920; N6P 2,340).
If the gate fails: one diagnosis pass (R5, read-only). Only for a wiring error against this text: one correction and
one repeat. Else commit the evidence ("FP2-C2: lab FAILED; default path not yet proven"), stop and wait.

## 6. The order of the work

1. One whole-item source check (R4). Then the change of section 3 and the fixtures. Build `SU2_CFD` and
   `UnitTests/test_driver` without a new warning. Fixtures of section 4; all fixtures of FP1 and the cohort of part
   1; G1 identical (375 / 114 / 228). Commit and push ("FP2-C2: fixtures passed; default path not yet proven").
2. The lab of section 5. Commit and push ("FP2-C2: lab passed; default path not yet proven").
3. One run S (both options YES): a copy in `FP2_C2/` of `FP2_C1/run_fp2_b_g5.py` (mode S) with the prerequisites of
   this item (the fixture receipt of item 1, the lab acceptance of item 2), the config `FP2_C1/run.cfg` (SHA256
   `07b91a94...5382`), the work folder `FP2_C2/G5/build<N>/S/`. All standing gates of both events. The six files are
   not a gate (the mesh changes by design); write their SHA256. Report beside the accepted run S of part 1, for each
   event: the native maximum; the cells, points and proof faces of the final mesh; the row `NATIVE3D_LOCAL_SURFACE`
   with the new fields; the engine's collapse pass (seconds, rounds, `collapse_accepted`, and the surface points
   that it removes, as `engine_reference()` of `FP2_C2_0/report_c2.py`: `faces` of the first
   `NATIVE3D_LOCAL_SURFACE_STATE` row of the event minus `working_faces` of `NATIVE3D_STAGE ...
   stage=length_collapse_exit`, divided by 2); the engine seconds of construction, of the quality phase and of each
   geometry repair; q / J at `quality_entry`; the table "where the time is".
4. Commit and push the evidence ("FP2-C2: S passed; default path not yet proven"), the report
   `R/night_20261115/FP2_C2/reports/FP2_C2_<build>.md`, the progress file, `HANDOFF_Codex.md` and `HowToProceed.md`.
   Verify that no owned job runs. STOP and wait for the user. Do NOT start a default proof or a second run S.
In this item no step reverts (this user order replaces the revert of R5 for this item). At a failed gate: one
diagnosis pass (R5, read-only); a wiring error against this text gets one correction and one repeat; then commit the
evidence with "FP2-C2: <step> FAILED; default path not yet proven", update the progress file, verify that no owned
job runs, STOP and wait.

## 7. Not in this item (the debt that stays)

An outward collapse (it needs a contact test). The collapse of a feature point or of a corner. The collapse of a
border point or onto a border point. A surface swap or move. A certified bound of the reverse distance in a trial.
The strict bound (`EntityFaceError`) in a trial: the final proof makes it for all faces. A change of the engine (its
collapse pass stays).
