# Fast path stage 2, item FP2-C part 3: the core removes sheet points also when the surface moves OUTWARD (contact test)

Author: reviewer, 2026-11-23 (source check and design review by Codex). For the main agent. The user sends it; it
is a user order. Copy this file with its SHA256 into `R/night_20261115/FP2_C3/inputs/`.
Code state: start only after the exact label "FP2-D: S passed; default path not yet proven" is committed. S0 = that
commit. Write in the progress file S0, its build, its lab acceptance receipt and its passed receipt of run S. With
any other label or a missing receipt: do not start; write the state; stop and wait for the user.
Short names: `Core.hpp` = `Common/include/adaptation/CNativeLocalCore3D.hpp`; `Surf.hpp` =
`CNativeLocalSurface3D.hpp`; `Col`, `Surface`, `Pass`, `Mesh`, `Shift` =
`Common/src/adaptation/native_local/CNativeLocalCore3D<name>.cpp`; `SG` =
`.../native_local/CNativeLocalSurfaceGeometry3D.cpp`; `Run` =
`Common/src/adaptation/native_engine3d/NativeRemeshRun.inc`; `Dist` = `Common/src/adaptation/CNativeDistributed3D.cpp`;
`Pred` = `Common/src/adaptation/CNativePredicates3D.cpp`; `VH` =
`UnitTests/SU2_CFD/adaptation/fixtures/native_volume_handover.inc`; `Lab` =
`UnitTests/SU2_CFD/adaptation/CNativeLocalCoreLab_tests.cpp`. Production line numbers are those of `240330d3aa`
(build654 solver); FP2-D moved some of them: find a place by its name.
The helper-watchdog rule (as `FP2_C1_CONTINUE_2.md`): if a helper watchdog ends a step and every gate that was
evaluated before it passed, keep the attempt and the host load, and repeat that step one time with three times the
helper timeout. If the repeat ends the same way, the step is OPEN for the user: keep the code, stop and wait. All
comparative timings and the 600 s value of the lab are reports. The 5400 s gate of each event and the limits of R5
and R6 stay.

## 1. Why

Step C (part 2) accepts a collapse only when no new face is outside the old cells (the INSIDE rule). Your run S of
part 2 shows the result: step C removes about 7,000 points in an event, the refusal `Outward` is the largest one
(21 to 39 thousand for each rank and pass), and the engine still removes about 2,500 surface points in each event
in its slow collapse pass. The dry measurement of step 0 said the same: 89 % of the directions that pass all other
tests move the surface outward.
An outward collapse adds a thin region (a "cap") to the mesh. On a thin body part the cap can go through the other
side of the body. So an outward collapse needs a CONTACT TEST. This item adds it:
- on one rank the test is exact: each new cell against each other new cell and against each kept cell near it,
  with the function `ConformingDisjoint` (what the engine does in `ClearanceBatch`, `Dist:455-459`, `520-547`);
- the cells of the other ranks are not known in a pass. A box test against the surface of the other ranks (as it
  was at the start of the pass, with a margin) refuses a collapse near them;
- an exact check of the whole mesh at the end of each pass (each cell against each cell that has a physical face,
  on all ranks) is the authority: a contact that the tests did not prevent ends the event with a clear failure
  before the mesh goes on. It also sees a contact that a surface split made (the split has no contact test).
The step-0 estimate for one sweep with outward collapses is about 9,600 points (N1) and 9,800 (N6P, pass 2); with
the inside rule it was 1,900 and 2,300.

## 2. Limits of the item

Production change in the local core (step C, a new contact module), its switch and the calls in `Run`. No change of
the engine, of `SplitSurface`, of steps L and D, of the volume steps, of `Limit()`, of the tolerance, of the
metric, of the tests 1 to 3 and 5 to 11 of step C or of their order. With only the new option off (3.1), each case
of S0 keeps its other options and gives its deterministic results of S0 (clock fields excluded; the new columns
print 0 where the row is printed; no gather is made; the admission path is that of S0). The default path (both
config options NO) and the run CORE do not reach the new code (keep the guards `Run:698-711`, `950-951`); the proof
of the default path is pending, as for part 2 and FP2-D. No new timer operation and no new field in `Session`,
`CellState` or a row of the engine that travels (R10). `SurfaceGeometry` gets no new function: the wrappers and the
fakes keep their interface. Readable code (D 7.1a): the contact module in new files
`Common/include/adaptation/CNativeLocalContact3D.hpp` and `.../native_local/CNativeLocalContact3D.cpp` (add the
source to the native-local list of `Common/src/meson.build`); one function for each test; a comment with the
reason at each test.

## 3. The change

3.1 Switch. Append `bool outward_collapses = false` to `CoreOptions` after `distance_error_cut`. It has an effect
only together with `surface_collapses`. `Run` sets it beside the surface options that it sets today. `VH` sets it
ONLY for the new lab selectors of this item; every selector that exists on S0 keeps its S0 settings.

3.2 Notation (as part 2, `FAST_PATH_STAGE2_C2.md` 3.2). For a changed face (p, a, b) with its old cell
X = (p, a, b, c) and its new cell N = (k, a, b, c), the CAP is the tetrahedron C = N with c replaced by p (the
vertex order that test 4 builds today, `Col:236-239`). The cap is the region that the face sweeps when p moves on a
line to k. A cap is OUTWARD when it is exactly positive (`Positive`): its interior is on the other side of the old
face than the old cell. A direction is OUTWARD when it has at least one outward cap; else it is INSIDE.

3.3 Test 4 with the option on. `PrepareSurfaceCollapse` no longer refuses an outward direction: it stores the
outward caps (vertex slots) in the trial (`SurfaceCollapseTrial`). With the option off, test 4 refuses with
`Outward` as today. With the option on, `c_refused_outward` is 0.

3.4 The contact tests. Two new values of `SurfaceCollapseRefusal`, appended after `Bound`: `ContactRemote` and
`Contact`. Extend `SurfaceCounters::c_refusals` and the reason-name array of `PrintSurfaceFields`;
`SurfaceSweepOutcomes` (the split type) does not change. With the option on they are evaluated in
`Mesh::CollapseSurface` in the lazy group (the function `test_geometry`, `Col:307-333`), BEFORE test 9, for EACH
direction that reaches the lazy group (inside directions too), under the same lazy rule as tests 9 to 11:
(R)  `ContactRemote`, only when the pass has a remote set (3.6): the closed box of an outward cap of the direction
     meets a box of the set. An inside direction has no outward cap and passes.
(N1) `Contact`: `ConformingDisjoint` is false for a pair of new cells of the plan (each unordered pair one time).
(N2) `Contact`: `ConformingDisjoint(N, Y)` is false for a new cell N and a live cell Y of this rank that is not an
     old cell of the plan (`plan.old`) and whose closed box meets the closed box of N. Y is ANY kept cell: also a
     cell with no physical face and a cell at a rank border. 3.5 gives the candidates; test the stored box again
     before the exact call (as `Dist:531-537`).
Evaluate (R) first, then (N1), then (N2). A refused trial changes nothing. The cells for the predicate: the
vertex order of the plan (`plan.fresh`) and of the cell row, the `Node` of each vertex (point id and coordinates);
the cell id is for messages only. Each new cell is exactly positive (test 8 comes before), each kept cell too.
The rank of a failure (which reason `CollapseSurface` returns when both directions fail) follows the order of
execution: the list of part 2 up to `Validity` of test 8, then `ContactRemote`, `Contact`, `Distance`, `Entity`,
`Bound`. The enum values are appended, so with the option on the rank is no longer the enum value: give it by one
small function. With the option off the returned reasons and their ties are those of S0.
Comment with the reason (write it at the tests):
"The mesh of this rank before the trial is embedded and conforming. A collapse changes only the pairs (new cell,
new cell) and (new cell, kept cell). Two cells with separate closed boxes do not touch. Each other changed pair is
tested with the exact predicate of the engine: interiors disjoint, and each contact in the simplex of the shared
points. The pairs (kept, kept) do not change. So the mesh of this rank stays embedded and conforming after the
commit, and by induction over the commits of a sweep, when the candidates of (N2) come from the CURRENT mesh. The
proof of part 2 for an inside direction (degree argument) gives the volume count; this test also covers a contact
of zero volume, for inside directions too.
Where the volume goes: as chains, (new cells) = Star(p) + (sum of the caps with their signs). Away from the cell
boundaries, the new volume outside Star(p) is contained in the outward caps. So (R) uses those caps as the
proximity screen for the other ranks. The check at the end of the pass (3.7) also covers a contact of zero volume
and a direction with no outward cap. The input of step C is embedded only if steps L and D kept it so (a split has
no contact test); that check sees it."
And at (R): "Three tolerances is an empirical screening margin, not a proven bound (the idea: a face that stays
within one tolerance of the model moves at most about two tolerances; one more for what is only sampled). A
gathered face that shares a point with the cap is a hit. A miss proves only that the cap is separate from the
grown boxes of the faces as they were at the start of the pass. The faces of another rank can change during the
pass, also at a border point (the border points themselves stay). The check at the end of the pass (3.7) is the
acceptance check."
A named constant `ContactMargin = 3.0` (in units of `Tolerance()`) in the contact header. Grow each box outward
with the rounding on the safe side (the grown box holds the exact one).

3.5 The local candidates (owned by step C, `SurfaceCollapseWork`; built only when the option is on). An index over
ALL live cells of the pass mesh: one entry (slot, the four vertex slots, closed box) for each cell, and a local
`CADTElemClass` over the boxes (the box-as-LINE construction of `Dist:425-435`: `globalTree = false`, dense local
indices). A query returns the entries of the tree whose box meets the query box, plus the entries of a list
`fresh` whose box meets it (scan `fresh` one time for a direction with the box around all its new cells, then test
the single boxes). An entry is used only if its slot is live and still holds the same four vertices (a
dead or reused slot is skipped; step C creates no point, so a vertex slot is not reused in the step). Sort the
candidates of a query by slot and remove duplicates before the exact calls, so that the first failure and the
count of calls are the same in each run. The tree's query changes its own work arrays: make the queries on the
owning rank in the serial order of the sweep, never from two threads.
After each commit of step C: append each new cell to `fresh` (expose the slots that `Commit` assigns, after all
links are installed: `Commit` returns them or records them for the caller; its result for the other callers does
not change). Build the tree one time at the start of step C and keep it over the sweeps. Rebuild it from the current mesh (and
clear `fresh`) only when `fresh` has more entries than the larger of `ContactFreshLimit = 4096` and one sixteenth
of the tree entries (release the old tree before the new one is built). The live-entry test above runs at each
query. Nothing else changes a cell during step C.
`CollapseSurface` gets the index and the remote set (3.6) through one optional pointer parameter after `sink` (a
small context struct; null = no contact data). Thread it through both `RunPass` overloads, `SurfaceWork` and
`SurfaceCollapseWork`. In (N1), first leave out the pairs with separate closed boxes, then test each other pair
one time. With the option on and a null pointer, each direction that reaches the lazy group is refused
`Contact` (a direct caller without contact data must not accept). The existing direct callers keep the defaults
(option off).
The refusal memory and the work list of step C do not change: a pair that was refused with `ContactRemote` or
`Contact` is tried again when one of its two ends changes, as for every other reason. A far cell that blocked a
collapse and goes away later does not bring the pair back in this pass. That is a loss of yield, not of validity;
say it in a comment.
Storage. BEFORE the rank-local `RunPass`, in the collective admission of the pass (`budget.Require` of `bytes`,
`Run:974-983`), reserve a contact peak that is valid up to the cell limit of the pass (steps L and D can make the
mesh larger than the input of the pass):
- for each cell: the entry, the box and the share of the tree. `SurfaceArrayBytes` gets a third parameter
  `bool contact_index = false` for these bytes;
- fixed bytes (a separate function of the contact header): the tree by
  `CADTElemClass::PredictBytes(3, 2 * n, n, 2 * n, &retained, &constructorPeak)` for n = the cell limit (it gives
  values for a whole tree, with fixed parts for each thread: do not call it for one element and multiply); the
  coordinate, connectivity and id arrays of the build; the constructor peak; the hit buffer and the growth of the
  tree's traversal arrays in a query; the two direction trials; `fresh` at its rebuild limit plus the new cells of
  one commit.
The cell limit of the pass: for a candidate limit n compute the complete modeled peak of the pass at n (the
caller and overlay bytes, the meetings, the live screen set of 3.6, the mesh and surface reservations of today,
and ONE complete contact reservation with the tree sized by `PredictBytes` at n; count each array and work space
one time: the constructor peak already holds the retained tree, the caller arrays and the growth in a query are
separate). Take the greatest n that is admitted, with checked arithmetic; then admit collectively before
`RunPass`. If the input count of the pass does not fit: the existing admission failure. The contact reservation is
one time in the caller baseline of the geometry queries of the pass (`SetCallerBytes`; reset it inside the
existing rank-local failure wrapper, it can throw).
Step C itself must NOT call `MemoryBudget::Require` (it elects collectively; a rank-local step must not). A later
rank-local capacity check may throw into the existing election of the pass. `Lab` / `VH` use the same accounting
where they set the option (in `VH` with the serial / parallel formulas of the error table of FP2-D). With the
option off the admission is that of S0. Write the added bytes in the report.

3.6 The screen against the other ranks (parallel only; nothing of this subsection runs with one rank or with the
option off). New in the contact module:
- A private row with `Fields(stream)`: the `Cell` of an owned row (`OwnedCell::cell`), the index k of a physical
  face (`physical[k].present`), the source rank as a fixed-width integer. `OwnedCell::Fields` does not change.
- `RemoteSurface GatherRemoteSurface(World&, const Owned&, double margin, MemoryBudget&, size_t caller, bool report)`
  (collective). Each rank sends to all ranks one row for each physical face that it owns (use `SendEverywhere` and
  `LocalStep` of `SG:345-373`; move the two templates to an internal header beside the sources so that both files
  see them; the existing calls behave as before). Each rank keeps the rows of the OTHER ranks, sorted by source
  rank, cell id and face index; for each row the closed box of the three points of the face, grown by the margin;
  and a local `CADTElemClass` over the boxes. Every allocation (send lists, rows, boxes, tree, hits) is admitted
  through the budget before it is made; `Bytes()` gives the resident bytes. A rank with no row of another rank has
  an empty set, not a null one.
- `bool RemoteSurface::Near(box)` (not const: the tree query changes its work arrays; call it serially): a stored
  box meets the given closed box (test the stored box again after the tree query). This is test (R).
- In `Run` (`RunLocalCore`, the pass loop `Run:970-1082`), only when `surface` is set, `workers.size > 1` and the
  option of this item is set: at the start of each pass, after `MeetBorderPoints` and before the admission of the
  pass: `GatherRemoteSurface` with `margin = ContactMargin * surface->Tolerance()`. While the set lives, its bytes
  are in `bytes` and in the caller bytes (3.5). Give the set to `RunPass` (a new last parameter of both overloads,
  `RemoteSurface* remote = nullptr`, which the context of 3.5 carries to step C). Free the set right after
  `RunPass`.
- The pass mesh keeps the constructor argument `allow_border` (today it only goes to `BuildFaces`). With the option
  on, a pass mesh that was built with `allow_border` (the parallel sign, also when the rank has no border point)
  and has no remote set throws at the start of step C.

3.7 The check at the end of each pass (EVERY pass with the option on: one rank too). New in the contact module:
`PassContactCheck VerifyPassContact(World&, const Owned&, MemoryBudget&, size_t caller, bool report)` (collective
when there is more than one rank).
- The OWNER rows: one row for each owned cell that has at least one physical face (the `Cell`, the source rank;
  one row for a cell, also when it has two physical faces). With more than one rank each rank sends its rows to
  all ranks (`SendEverywhere`); each rank then holds the owner rows of ALL ranks, its own included, sorted by
  source rank and cell id, and a local tree over the closed boxes of the four points of each row's cell.
- Each rank tests EACH cell that it owns (also a cell with no physical face) against each owner row whose closed
  cell box meets the closed box of the owned cell, except the row of that same cell:
  `ConformingDisjoint(owned cell, row cell)` must be true. Two cells that share a face, an edge or a point (equal
  point ids and equal coordinates) pass the predicate, on one rank and between ranks.
- The first false answer (in the order of the owned cell id, then of the rows) or a local exception is a failure
  of the pass ON EVERY worker communicator, size one included: put it into the existing failure record of the
  pass and throw the elected `CElectedFailure` (with one rank the `LocalStep` of `SG` calls its action directly: a
  plain exception that leaves the call would end the program, not reject the pass). The message has the two cell
  ids and the source rank when a pair is known ("Native local surface contact: ..."). A contact failure stays on
  the rejection path of the pass.
- The function returns for the report: the owner rows and their resident bytes, the cells tested, the candidate
  pairs, the exact calls, the seconds of the gather and of the check. Every allocation is admitted through the
  budget before it is made (the caller baseline here: the current owned map after `WriteBack`, the current
  overlay and the meetings); elect a failure after each local allocation, build, packing or check phase, before
  the ranks enter the next collective call.
- Comment: "What this check proves. `Check` of the pass gives exactly positive cells and matched inner faces on
  each rank. ASSUMED: the partition of the input pairs each triangle between two ranks exactly one time, with
  opposite orientation and equal point ids and coordinates; the pass does not change these triangles, and
  `WriteBack` keeps the checked geometry and the physical bindings. On a way from outside the mesh to a place that
  two cells cover, the matched inner faces cancel in the count of the cells at a point; where the count first
  rises above 1, a cell that the way enters through a physical face overlaps another cell. For a contact of zero
  volume that is not permitted: take the smallest simplex of one cell that holds a contact point outside the
  shared simplex. If it is an inner simplex, the cells around it cover a full neighbourhood, so there is an
  overlap with volume. Else a cell with a physical face holds it and shows the contact. The closed boxes of the
  two cells meet in each case. The check also sees a contact that a surface split of step L or D made (the split
  has no contact test in its trial)."
- In `Run`: after `WriteBack` and the failure election of the pass (`Run:1044-1057`), BEFORE `Exchange` and
  `ShiftBorders` (one rank: before the hand-over), when `surface` is set and the option is set. The check must
  run on the partition of the pass. A failure ends the event here. Count the peak in
  `stats.maximum_native_bytes` as the other steps do.
A failure of this check in a lab trial or in the run S is a failed gate of that step (section 6); it is not a
wiring error unless your code differs from this text.

3.8 Print only (under `report`).
- In `PrintSurfaceFields`, right after `d_nonfinite` and before `c_changes` and the other quoted fields:
  `c_outward_tried` (outward directions that reached the contact tests), `collapses_outward` (accepted collapses
  with at least one outward cap), `c_refused_contact_remote`, `c_refused_contact`, `c_contact_directions`
  (directions that the contact tests evaluated), `c_contact_candidates` (pairs whose closed boxes met),
  `c_contact_calls` (calls of `ConformingDisjoint` in step C), `c_contact_rebuilds`, `c_contact_fresh_max`,
  `seconds_c_contact` (all of the contact tests), `seconds_c_contact_build` (the tree builds in it). The fields of
  part 2 and of FP2-D stay where they are. With the option off the new fields print 0.
- New rows (`Run`): `NATIVE3D_LOCAL_CONTACT rank= stage= pass= kind=screen remote_rows= remote_bytes= peak_bytes=
  seconds_gather=` for each gather of 3.6, and `NATIVE3D_LOCAL_CONTACT rank= stage= pass= kind=verify owner_rows=
  owner_bytes= peak_bytes= verify_cells= verify_candidates= verify_calls= seconds_gather= seconds_verify=` for each check of
  3.7.

## 4. Fixtures (hidden tags `[.][NativeFP2C3]`, so that the case table of G1 stays identical)

The embedding check (test code, for the fixtures and the lab): `EmbeddingViolations(mesh)` builds a static tree
over the boxes of ALL live cells of a core mesh and tests each unordered pair of cells whose closed boxes meet one
time with `ConformingDisjoint`; it returns the count of false answers, the pairs and the calls. Test it first on
its own: a valid mesh gives 0 (cells that share a face, an edge or a point pass); a mesh with one cell replaced by
a cell that goes through a neighbour gives a count above 0.
Set-up where not said: mesh S33 of part 2; fake geometry with tolerance 0.5 (so `Limit()` = 0.35), `Dimension` 0
for k and 2 for every other point, `Samples` zero, `OnEntityFace` true, `WithinTolerance` true, a `Remove` that
records its ids; `shape_test = false`. Option-on cases use the fixture friend (`SurfaceFixtureAccess`) or an
extended copy of the C2 collapse helper; they give a contact context. Derive each coordinate in a test comment;
pin no value that you did not derive; if your derivation differs from an expected result below, write it in
`QUESTIONS_FOR_REVIEWER.md` (R13) and pin your derivation. For each trial that is meant to reach the contact tests, assert
FIRST that its direction passed tests 1 to 8 and reached the contact test that the case names (the option-off
control stops at test 4; the case with no context fails before a trial). In each accepted collapse below:
`Check` passes, each cell is exactly positive, `EmbeddingViolations` is 0 and `VerifyPassContact` passes.
(a) The tent: p = (1, 1, 0) moved to (1, 1, h), k = (2, 1, 0). Expected by the review (verify it by hand): a ring
    of three cells, nine new cells, four children in the plane z = 0 with smallest altitude 1 / sqrt(5) (above
    0.35), each reverse witness within |h|. h = +0.2 (toward the cells): option off `Outward` (it stops at test 4);
    option on, serial: accepted, `collapses_outward` 1, `c_refused_outward` 0; the region under the tent is now in
    the cells. h = -0.1 (an inside direction): accepted with the option on, `c_contact_calls` above 0,
    `collapses_outward` 0. h = 0 (p on the plane of each child): accepted.
(b) The bubble: the mesh of (a) with h = +0.2 plus ONE more cell B with four new points and new ids (a second
    component; all its faces physical). (b1) B strictly inside one outward cap (shrink that cap about its
    barycentre): refused `Contact` by (N2). (b2) B moved to z below 0 with no point in a closed new cell: accepted.
    (b3) B below z = 0 with one vertex exactly on the plane z = 0 inside a child face and the other three below,
    not in one plane with it: refused `Contact` (a contact that is not a shared simplex). For (b1) and (b3) build
    the mesh after the collapse in test code: `EmbeddingViolations` is above 0 and `VerifyPassContact` fails (the
    checks see what the test refuses).
(c) (N1): a fan in which all new cells are exactly positive and two new cells that are not neighbours touch or
    overlap: refused `Contact`, with the earlier tests passed (show it). If you find no such fan in the time of R6
    after an honest search, write the search in the report and test the function of (N1) directly with a hand-made
    plan of two positive cells that share only k and overlap. And a mixed fan (one cap outward, one cap strictly
    inside) with no contact: accepted.
(d) The index: (d1) a query returns a cell that an earlier commit of the same sweep CREATED (from `fresh`), and
    after a rebuild from the tree; (d2) a cell that an earlier commit REMOVED is not used, also when its slot holds
    another cell now; (d3) with the rebuild limit set to 1 by a test-only parameter of the context, the results of
    (a) to (c) do not change, and `c_contact_rebuilds` rises in a case that commits enough new cells to pass the
    limit (a refused trial adds nothing to `fresh`).
(e) Two ranks, the mesh of (b), globally distinct ids: rank 0 owns S33, rank 1 owns B. Build both pass meshes with
    `allow_border = true`, keep all bindings of the outer faces, and assert that no rank has a border point. Drive
    the selected collapse through the fixture helper with the REAL gathered set (do not let a full pass edit the
    set-up first). B inside the cap, and B outside every new cell but with its face box nearer than the margin to
    a cap box: refused `ContactRemote`. B farther than the margin: accepted, and `VerifyPassContact` on the output
    passes on both ranks. Pin the box gaps against the real margin (3 x 0.5). And through step C without a remote
    set on this mesh: it throws (3.6).
(f) `VerifyPassContact`. (f1) two ranks, a valid mesh on each, a cell of rank 1 goes through a physical face of
    rank 0: `CElectedFailure` on BOTH ranks, the message has the two cell ids. (f2) rank 0: cut one cell at an inner point, then cut one child at a second inner point; take a
    grandchild with no face of the first cell's boundary; rank 1: a small cell with all faces physical and new ids
    strictly inside that grandchild. Assert that the cell of rank 0 has no physical face and that the elected pair
    names it and the owner of rank 1.
    (f3) a partitioned S33 (shared border faces, edges and points): no failure; the counts are equal in two runs.
    (f4) the meshes of (f1) moved apart: no failure. (f5) ONE rank, the split case: the cell A = (0, 0, 0),
    B = (2, 0, 0), C = (0, 2, 0), D = (0, 0, 2) with the edge A-B split at M = (1, -0.1, -0.1) (two positive
    children), plus a small second component with new ids strictly inside the child (B, M, C, D) near
    (1, -0.064, -0.064), all its points at y < 0 and z < 0 (outside the old cell): the check fails. This is the
    contact that a surface split can make today; build the two children in test code.
(g) The outward tent of (a) on two ranks: give rank 1 only the cell (v0, v6, v3, v7) of the cube at
    (x, y) = (0, 1), with its bindings (find it by its points if your numbering differs, and write it). Rank 1 then
    owns the physical face (v0, v3, v7) at the rim point (0, 1, 0). Assert that this rim point is a border point,
    that p and k are not, and that the selected direction reaches (R) and is refused `ContactRemote`.
(h) A refused trial (`ContactRemote` and `Contact`) leaves the points, the cells, the bindings, `frozen_` and the
    overlay equal (as fixture (g) of part 2), and the index gives the same answers before and after it.
(i) Work list on = work list off (the same committed cells and faces) on the mesh of (a) with two more tents.
(j) With only `outward_collapses` off: the surface fixtures of FP2-A, FP2-B, FP2-C (the corrected serial, border
    and exchange cohorts of part 2 included) and FP2-D with their S0 options. Their deterministic results equal
    those of S0, and the new fields are 0 wherever the row is printed.
If a fixture reaches the limit of R6: write its state, finish the others, the cohorts and G1, commit the evidence
with "FP2-C3: fixture (<x>) OPEN at the R6 limit; default path not yet proven", update the progress file, verify
that no owned job runs, STOP and wait.

## 5. The lab (after the fixtures; BEFORE a full run)

Run N1, N6 and N6P two times each with the production `RunPass`, their lab inputs and options of the FP2-D lab at
S0 (steps L, D with `distance_error_cut`, C) and `outward_collapses = true` (new hidden selectors, 3.1; the bound
observer of the C2 lab stays attached). The lab pass loop of `VH` makes the same calls as `Run` (3.6, 3.7) at the
same places, outside its rank-local step wrapper (`CheckLabStep`), because they are collective.
Reference for each case: the sealed measurement of repeat 1 of the FP2-D lab at S0, loaded with the S0 comparator
(`FP2_D/compare_lab.py::load_trial`) bound to its own runner, with its recorded watchdog repeat when there is one.
Verify its build, case, receipt and artifact pins against the lab acceptance receipt of S0; that receipt must hold
the passed alarm and the N1Invariant of FP2-D (historical qualification of S0).
Runner and comparator: copy the S0 versions of `FP2_D/run_c2_lab.py` and `FP2_D/compare_lab.py` into `FP2_C3/`.
This item has NO alarm and NO N1Invariant trial: remove these two requirements from the copies (the comparator
must not ask for them, the receipt must not claim them, and the S launcher of item 3 must not assert them). Seal
the three cases with their repeats, the contact and embedding gates below, and the fixture receipt. Extend the
parsers and the comparator: the new fields of 3.8, the contact rows with their count for each rank and pass (one
`verify` row for each rank and pass of every case, N1 included; one `screen` row for each rank and pass of a
6-rank case only). A contact failure that a log shows
is a result failure in the lab runner and in the S launcher, not a reason for a watchdog repeat; a row that a gate
needs and that is missing at a normal end fails the qualification.
Conditions (gate), for each trial:
- each cell exactly positive; `Check`; the donor scan 0; the state build accepts and (6 ranks) the bytes of
  `Snapshot()` are equal;
- the complete qualification of FP2-D at S0 (`qualified=1`, `classify_unexplained=0`, `model_gap_entities=0`,
  `strict_checked` equal to the count of physical faces of the rebuilt state, `strict_sink_mismatch=0`, zero
  violations of the collapse probe, `strict_above` included; the altitude condition of the last pass; no core
  metric edge below 1e-2; `star_max` not above 1,000; `sweeps_distance` below 40; `c_sweeps` below 50;
  `v_limit=0`; `d_marks_error + d_marks_longest == splits_distance`). Test 11 does not prove that the census of
  the rebuilt state agrees for each face: `strict_sink_mismatch=0` stays the gate for that;
- `EmbeddingViolations` (section 4) of the pass mesh right after `RunPass`, for each rank and pass: 0. Admit its
  storage apart; report its pairs, calls, peak bytes and seconds apart from the pass seconds;
- the contact rows of each rank and pass are there (N1: `verify` only), each `verify` row has `verify_cells`
  equal to `cells_out` of the pass of that rank; no contact failure;
- each surface row has the new fields of 3.8, not negative, with `c_refused_outward == 0` and
  `collapses_outward <= collapses_surface`;
- both trials of a case qualify and are equal in the exact rows of the FP2-D comparator (the `FP2_C cells` digests
  included) plus the new integer fields of 3.8 and the integer fields of the contact rows (seconds and peaks
  excluded). The report-only fields of part 2 and of FP2-D stay report-only. No row and no mesh is compared with
  S0 as a gate.
A pass above 600 s is a report beside the host load.
Report beside the reference, for each case, pass and rank: the surface faces and points before and after step C;
the accepted collapses for each sweep (`c_changes`); `collapses_outward`; the refusals by reason; the contact
counters of 3.8 and the seconds for each accepted collapse (contact, bound, all of step C) beside S0; the contact
rows (rows, bytes, peaks, seconds of the gathers and of the checks); `classify_above_tol` and the excess bins; the
bad cells after V by class and the cells (what the engine gets: a rise is a finding, not a failure); the shortest
edge in space, the smallest face angle and the smallest face altitude over all physical faces; the modeled peak
and the conservative storage sum of part 2. For N6P beside N1: `c_refused_contact_remote` over `c_outward_tried`
as two integers (the cost of the margin and of the box test); and the seconds of `VerifyPassContact` apart, and as a share of the time of the pass up to the
end of the check (the lab's own embedding check is not in that time). And the surface points that step C removes in each
case beside the about 2,500 points that the engine still removed in each event of the run S of part 2.
If a gate fails: the failed-gate rule of section 6.

## 6. The order of the work

1. One whole-item source check (R4). Then the change of section 3 and the fixtures. Build `SU2_CFD` and
   `UnitTests/test_driver` without a new warning. Run: the fixtures (a) to (i); for (j) the option-off coverage of
   S0; all 18 FP1 cohorts; G1 identical (375 / 114 / 228). Seal the receipts. Commit and push ("FP2-C3: fixtures
   passed; default path not yet proven").
2. The lab of section 5. Commit and push ("FP2-C3: lab passed; default path not yet proven").
3. One run S (both config options YES): copy the S0 version of the S launcher of FP2-D (`FP2_D/run_fp2_b_g5.py`)
   into `FP2_C3/` and run mode S. Pin the passed `run_receipt.json` of the run S of FP2-D at S0 (follow
   `full_watchdog_retry` if the passing receipt was sealed there); verify its build, config and artifact hashes
   against the evidence of FP2-D; require the FP2-C3 fixture and lab receipts (no alarm, no N1Invariant). Keep its
   standing gates and its helper-watchdog logic; a contact failure in a rank log is a result failure. Config
   `FP2_C1/run.cfg`, SHA256 `07b91a94996bf3cb48352041d8e1643ffe17a61bed07432085cfcf87b1725382`. Work folder
   `FP2_C3/G5/build<N>/S/`. All standing gates of both events. This is ONE observation run by this user order: write the SHA256 of the six
   files; they are not a gate. The repeat with the same binary and the proof of the default path are NOT
   evaluated in this item: the label below is not an acceptance and makes no new reference.
   Report beside the run S of FP2-D at S0, from the sealed receipts, for each event apart: the native maximum; the
   cells, points and proof faces of the final mesh; the surface faces and points at the hand-over; the bad cells
   after V by class; the rows `NATIVE3D_LOCAL_SURFACE` and `NATIVE3D_LOCAL_CONTACT`; the core seconds of step C
   (all, contact, bound) and of the gathers and checks on the slowest rank, with their peak bytes; the engine's
   collapse pass (seconds, rounds, `collapse_accepted`, and the surface points that it still removes: `faces` of
   the first `NATIVE3D_LOCAL_SURFACE_STATE` row of the event minus `working_faces` of `NATIVE3D_STAGE ...
   stage=length_collapse_exit`, divided by 2; write "not available" where a row is missing); the engine seconds of
   construction, clearance, the quality phase and each geometry repair; q / J at `quality_entry`; the table "where
   the time is".
4. Commit and push the evidence ("FP2-C3: S passed; default path not yet proven"), the report
   `R/night_20261115/FP2_C3/reports/FP2_C3_<build>.md`, the progress file, `HANDOFF_Codex.md`, `HowToProceed.md`
   and `Handbook/ADAPTATION_TIMINGS.md` (the same evidence). Verify that no owned job runs. STOP and wait for the
   user; start neither a default proof nor a second run S.
The failed-gate rule of this item (this user order replaces the revert of R5 for this item): keep the code. For a
failed gate make one read-only diagnosis pass (R5); only a wiring error against this text gets one correction and
one repeat; if the repeat passes, go on. Else commit the evidence with "FP2-C3: <step> FAILED; default path not yet
proven", update the progress file, verify that no owned job runs, STOP and wait.

## 7. Not in this item (the debt that stays)

A proof of the margin of (R) inside a pass: the check at the end of the pass is the authority, and its failure
ends the event (no repair, no second try without the outward collapses). A contact test in the TRIAL of a surface
split (`SplitSurface` accepts an outward cap up to the tolerance with no contact test; the end check of 3.7 finds
such a contact, it does not prevent it). A cheaper end check: after the input is certified one time, test only changed cells against
all owners and changed owners against all cells (changes by the cell ids of `WriteBack`, kept over the shift); and
test a pair of two owners one time. A second try of a
pair after a far blocking cell went away. A
tighter distance than the box test for the faces of other ranks. The collapse of a feature point, of a corner, of a
border point or onto a border point. A change of the engine: its collapse pass stays (the flag "no construction
collapse in the engine" is a later item, after the numbers of this one). A reuse of the strict face answers by the
final proof. The acceptance of this item (default proof, two equal runs S).
