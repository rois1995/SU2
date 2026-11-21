# Fast path, stage 2, item FP2-A: the core splits the surface

Author: reviewer, 2026-11-21 (checked against the source). For the main agent. The user sends it; it is a user order.
Base: `abc9450b86` ("FP2-0b measure feature-edge splits and two-sided proof") or later.
All rules of section 1 of `AGENTS.md` apply (R1 to R13). R = `integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57`.
Evidence folder: `R/night_20261115/FP2_A/`. Copy this file with its SHA256 into `FP2_A/inputs/`.
Texts that this file uses (your copies): "S1" = `FAST_PATH_STAGE1.md` with its resumes (`FP1/inputs/`); "P0" =
`FAST_PATH_STAGE2_STEP0.md` (`FP2_0/inputs/`); "P0b" = `FAST_PATH_STAGE2_STEP0B.md` (`FP2_0B/inputs/`).

Short names. "The core" = the code of FP1 (`SU2NativeLocal3D`). "The engine" = the present 3D engine. "The state" = the
`BoundaryParameterState` of the event, `preparation.pending->state.original->Parameters()`. "tol" =
`state.GeometryTolerance()`. Files: hpp = `Common/include/adaptation/CNativeLocalCore3D.hpp`; Mesh, Changes, Pass,
Shift, Measures = `Common/src/adaptation/native_local/CNativeLocalCore3D<name>.cpp`; NRR =
`Common/src/adaptation/native_engine3d/NativeRemeshRun.inc`; EA = `native_engine3d/EngineAdapt.cpp`; BP / BPh =
`CNativeBoundaryParameters3D.cpp` / `.hpp`; AR / ARh = `CNativeAffineReference.cpp` / `.hpp`; CA / CAh =
`CNativeCurvedAtlas3D.cpp` / `.hpp`; DI = `CNativeDistributed3D.cpp`; GB = `CNativeGeometryBackend3D.cpp`; your test
files of FP2-0b: T2 = `UnitTests/SU2_CFD/adaptation/CNativeFastPathStage2_tests.cpp`, W =
`fixtures/native_fast_path_stage2_walk.inc`, S = `fixtures/native_fast_path_stage2_strict.inc`, SP =
`fixtures/native_fast_path_stage2_split.inc`. Line numbers are those of the base.

## 1. Why, and what this item is

1. FP2-0b answered the question of the measurement: plain `double` code split the surface of event 1 (17,762 splits in
   15 s on one processor), and the strict code accepted the result (0 of 84,478 faces failed, the bulk constructor
   accepted in 28.7 s, the two-sided proof accepted all four entities). Your code of FP2-0b is the first version of
   the code of this item.
2. Event 1 with the option of FP1 ON still needs 1,500 to 1,550 s, because the engine does all surface work (the
   phase `construction`: 487 s for each rank; most of the phase `quality`: 869 s). This item moves the SPLITS of
   surface edges into the core. After the core, each rank builds ONE new state with the validating constructor; then
   the engine runs without a change, as the checker and the repairer. All final gates stay.
3. Not in this item: a collapse, a swap or a move of a surface point; the end of the wall-near rule of S1 6.2. They are
   the next item.
4. The item has three parts, in the order A, B, C. Each part has its gates (section 11).

| Part | Content | Production effect |
|---|---|---|
| A | The surface split in the core for one rank; fixtures; the lab on the real input of event 1 | none (no call from production) |
| B | The ranks: the exchange of the new records after each pass; the gather; the build of the state | none (no call from production) |
| C | The option, the call in `RunLocalCore`, the full gates | new option OFF: identical results. ON: a new mesh |

## 2. The option and when the surface work runs

- New boolean config option `ADAP_NATIVE_LOCAL_SURFACE`, default `NO`, getter `GetAdap_Native_Local_Surface()`, made as
  `ADAP_NATIVE_LOCAL_CORE` was (its places: `CConfig.hpp:1322`, `:10496`, `CConfig.cpp:3310`, `:6449-6450`). No entry
  in `config_template.cfg`.
- The surface work runs in a call of `RunLocalCore` when ALL of these are true: the core runs (S1 2.3, not changed);
  the new option is YES (a local read); the state has `EntityGeometry()` and `Mobility().identity != 0` (the same on
  all ranks).
- `CoreOptions` gets a field `surface_splits`, default false. NRR sets it to true only under the conditions above
  (today NRR gives no options, NRR:812). With the new option NO, no statement of this item runs: no collective call,
  no timer scope, no allocation, no print, and each function of the core gives the result of the base bit for bit.
  Two exceptions, both without an effect on a result: the new rule of `Check()` (4.4; it holds in the base, because
  `BuildFaces` sets `surface` for each point of a physical face, Mesh:150-155) and the code of `Prepare` for an empty
  `replaced` (4.1).
- With the option YES and the state condition false: one row `NATIVE3D_LOCAL_SURFACE rank= applicable=0` under
  `control.timings`; the core runs as today.
- Do not change `PointRow`, `CellRow`, `FreshCell`, `FrozenPoint`, `EdgeUse`, `LengthCandidate`, `QualityCandidate`,
  `Edge` or `Face`: their sizes make `Mesh::BytesPerCell` (Pass:86-92), which is in the admission and in `cell_limit`
  (NRR:798-802), and so in the modeled bytes of G2 and GQ. The tables of the surface steps and the overlay are own
  members or own objects.

## 3. Files and build (part A)

All new sources go into `accurate_sum_lib` (`Common/src/meson.build:46-61`), namespace `SU2NativeLocal3D`. The geometry
code (AR, CA, BP, GB) is in the same library with the same strict flags, so the core calls it directly. The library
code of this item calls no function of the library `common` (for example `OwnedStorageBytes`,
`CNativeImport3D.hpp:58`: NRR calls it and gives the number). No pragma in production code. R9 applies (short
functions, one purpose each, comments that say why).

| File | Content |
|---|---|
| `Common/include/adaptation/CNativeLocalSurface3D.hpp` | the abstract class `SurfaceGeometry` (section 5), the struct `SurfaceEdits` and the function `BuildSurfaceState` (section 8) |
| `native_local/CNativeLocalCore3DSurface.cpp` | `SurfaceRing`, `SplitSurface`, the two surface steps, `MarkUnresolved`. It uses only the interface, so a box fixture can give a fake geometry |
| `native_local/CNativeLocalSurfaceGeometry3D.cpp` | `class StateSurfaceGeometry : SurfaceGeometry` over the state and the overlay (section 5): the production float walk, the 4 samples, the placement, the feature point and its traces; the exchange of section 7; `BuildSurfaceState` |
| Mesh, Changes, Pass, hpp | the small changes of sections 4 and 6 |

`OrientedFace` and `FaceSign` are in the anonymous namespace of Mesh (Mesh:15-21). Declare
`Face OrientedFace(Vertices, unsigned)` in hpp (the same body, outside the anonymous namespace): `SplitSurface` and
section 8 use it.
The float walk exists only as test code. Write it again in `CNativeLocalSurfaceGeometry3D.cpp` from your `Walker`
(W:23-261: the facet table W:25-45, the start rules `Start` W:47-68 and `EdgeStart` W:70-84, `Run` W:86-138 with its
cap of 64 facet changes, the snapping W:236-243 and `Binding` W:140-154, `Samples` W:156-170);
`detail::MinimizeTriangle` is then compiled with the strict flags. The test files of FP2-0 and FP2-0b need not stay
runnable; you can make them call the production walk.

## 4. The change in the volume: `SplitSurface` (part A)

### 4.1 `Prepare` with replaced physical faces
`Prepare(old, fresh, replaced = {})` (Mesh:165-239). An entry of `replaced` = a parent face, given as (old cell slot
`c`, face `k` with `neighbors[k] == Physical`), and the slots `a`, `b`, `m`. `Prepare` takes the parent triple
`OrientedFace(cells_[c].vertices, k)`; child 1 = that triple with `a` replaced by `m`, child 2 = that triple with `b`
replaced by `m` (replacement by position, in the oriented order, as the new cells are made).
AFTER the loop over the old cells (the loop is Mesh:174-190; the place is before `std::map<Face, FaceUse> internal`,
Mesh:192), for each entry: the outer face `Sorted(parent)` must exist with `cell == Physical` and the sign of the
parent; erase it; insert child 1 and child 2 as outer faces `{Physical, 0, the binding of the parent, FaceSign(child)}`;
a child key that exists already: refuse (`Topology`). Then the present matching (Mesh:205-230) runs without a change:
each child is met exactly one time by a face of a new cell with the same sign, and that cell takes the binding.
`Commit` does not change.
`Prepare` skips the test `Blocked` of Mesh:177-180 exactly when `replaced` is not empty. (A long surface edge makes its
ends `unresolved`, Measures:108-110; with the test, each surface split would be refused.)
Write the reason as the comment: the four children of the two parents have the same border edges as the two parents, so
the oriented boundary of the new cells is the oriented boundary of the cavity with each parent replaced by its
children. Both physical faces of the edge must be in `replaced`; with one only, the other parent stays unmatched and
the change is refused. The two parents can have different anchors; each child keeps the binding of its own parent.
With an empty `replaced` the function is the function of the base.

### 4.2 `SurfaceRing`
For an edge (a, b): the cells around the edge (as `EdgeRing`, Changes:116-134) and the list of the (cell, face) pairs
with neighbour `Physical` that contain both a and b. Eligible: no `Border` face in the walk and exactly two such pairs.
A `Border` face in the walk: the result "border" (the callers count `left_border`). Another count of pairs:
`Ineligible`. `EdgeRing`, `ClosedRing` and `Split` do not change.

### 4.3 `SplitSurface(edge, seed, geometry, point_ids, stamp, cell_limit, stats, progress_test)`
In this order; the first step that fails ends the try with its count, and nothing is changed:
1. `SurfaceRing` (4.2). `SplitSurface` does not call `Blocked`.
2. `Fits(n, 2n)` as `Split` (Changes:247); else `refused_memory`.
3. Placement: `geometry.Place(...)` (5.2) with the id `point_ids.Peek()`. It gives the `BoundaryPoint` of `m`
   (coordinates and binding) and, for a feature edge, the two child traces. Refusals: 5.2.
4. Outward guard (float). For each of the two parents, with `(p0, p1, p2)` = the parent triple:
   `h = -((p1 - p0) x (p2 - p0)) . (m - p0) / |(p1 - p0) x (p2 - p0)|`; `h > 0` is the side of the opposite point of
   the cell that holds the parent. If `h < -tol` for one parent: refuse (`refused_outward`). Keep `outward_max` = the
   largest value of `max(0, -h)` over the accepted splits, for the print. Reason (comment): when `m` is on the cell side of both parents
   and all new cells are positive, the new cells are inside the cavity. When `m` is outside, the new cells fill a cap
   outside the old mesh; a cap thinner than tol cannot reach another part of a body that the tolerance resolves. A
   larger outward step needs a contact test, which the core does not have yet; the engine does these splits.
   (Expected on M6: 0 refusals. In run A of FP2-0b `h` is between -2.5e-5 and +1.2e-4 on the wing and below 1e-13 in
   size on the symmetry plane, at the root curve and on the far field.)
5. With `progress_test` (step L only) and `options_.split_progress`: the test of `Split` (Changes:255-268) with the
   real `m`; it needs only a `PointRow` value.
6. The point row of `m`: coordinates from 3 (NOT the flat midpoint), tensor `Midpoint(a, b)` (Changes:23-26, as `Split`
   at Changes:254), `surface = true`. Sort the ring cells by `Key` as `Split` does (Changes:269-270) before the new
   cells are made. Take the slot and install the row as `Split` does (Changes:271-286). New cells = each ring cell
   two times, as `Split` (Changes:276-279). `Prepare(ring, fresh, replaced = the two parents)`, then
   `ExactlyPositive` (as `Split`, Changes:289). A refusal counts as in `Split`, in the surface counters.
7. Strict check of the four children: `geometry.OnEntity(child)` = `state.ContainsEntityFace(the anchor of the parent,
   the three BoundaryPoints in the oriented order of the child)` (BP:305-315). One fails: refuse (`refused_entity`).
   This is the exact test that the constructor makes for each face (BP:259-260).
8. A refusal in step 6 or 7 restores the slot of `m` (Changes:295-300). After acceptance, exactly as Changes:302-305:
   `RememberWorkPoints(plan)` (without it the work list of the next sweep does not hold the child edges); the pop of
   the free slot (only when the slot was used again); `point_ids.Take()`; `Commit`. Then `geometry.Accept(placement)`
   (5.3) and the append of `m` to `frozen_` (Mesh:108-109), so that the self-check of the pass guards it.
Use own counters for the surface (a struct `SurfaceCounters` in `PassStats`), not the counters of the volume.

### 4.4 Flags after the surface steps
`MarkUnresolved()`: a point that is not a `border` point gets `unresolved` = "it is an end of a live surface edge with
`Length > LongEdge`". A `border` point keeps its present value (it holds the mark of the meeting, Shift:221-229), OR
the new local value. Call it one time after the surface steps of a pass (section 6). `wall_near` is computed again by
each collapse sweep (Pass:158).
`Check()` (Mesh:322-358) stays; add: each point of a `Physical` face has `surface`.

## 5. The geometry of the surface (part A)

### 5.1 The interface `SurfaceGeometry`
The core calls only this interface. One function answers the question "is this near enough?": `Limit()`. Today it is
`SurfaceDistanceFactor * tol` with ONE named constant `SurfaceDistanceFactor = 0.7` in the new header (the user will
decide later on the tolerance and on a size from the curvature; such a decision must change only `Limit()`, the 4
samples, or the target, and no other code).

| Function | Meaning |
|---|---|
| `Samples(face)` | the four float distances of a surface face to the model (P0 row G1; your `Samples`, W:156-170): centre, then the midpoints of v0v1, v1v2, v2v0. Input: the three point ids with coordinates, the anchor facet. Keep the distance also when the walk did not converge (as SP:215-229) |
| `Chord(edge)` | for a feature edge: the distance between its flat midpoint and the point that `ProposeFeaturePoint` proposes (P0b 1.4 item 1; your `MarkFeatures`, SP:382-399); no point is made |
| `Feature(edge)` | the sorted pair of ids has a trace (P0b 1.1) |
| `Limit()` | see above |
| `Place(edge, the two parents, id)` | 5.2 |
| `OnEntity(child)` | 4.3 step 7 |
| `Accept(placement)` | 5.3 |

### 5.2 `StateSurfaceGeometry::Place`
- Not a feature edge (your `WalkPoint`, SP:479-503): the float walk from the flat midpoint with the start rule of P0
  row P (`EdgeStart`); where that rule says "the first face of the edge", take the parent with the smaller sorted
  triple of point ids. Binding by the snapping of GB:158-170; coordinates = `Realize(binding)`. Refuse: the walk did
  not converge, reached its cap or threw (`refused_walk`); `MobilityDimension != 2` (`refused_protected`).
- A feature edge (P0b 1.2; your `Propose`, SP:371-380, and `Rebuild`, SP:519-542): `ProposeFeaturePoint` (CAh:33-35)
  with the trace of the edge, then `RebuildFeatureTraces` (CAh:45-47) on the small `Patch`; take only the entries
  (a, m) and (b, m). A false return or an exception: `refused_feature` (with the first reason), `refused_trace`.
- Both kinds: refuse when the support key or the coordinate key of the new point is that of a known point
  (`refused_duplicate`). Known = each record of the state, each point of the overlay. Use the keys of the constructor
  (BP:242-243), as SP:568-577.
- The binding of an end point: the overlay first, then `state.Find(id)`.
- Budget: the object gets ONE `AtlasBudget` from its maker. In NRR (part C) it is the twin of the budget of the event
  (NRR:379-386): `maximum_bytes = kernel.memory_ceiling`; `caller_bytes = kernel.caller_bytes +
  OwnedStorageBytes(working)` + the bytes of the object; `maximum_integer_bits` by the loop of EA:1063-1071 (write
  it as a small function of the new header with the literal 16 MiB, `size_t{16} << 20`, as T2:93: `AtlasScratchBytes`
  is anonymous in `CNativeEngine3D.cpp:304`);
  `maximum_work = (stats.initial_cells + control.maximum_work) * MappedFacetReference::ClearanceWork` (NRR:382-384;
  ARh:193);
  `within_time = [this] { return WithinTime(); }` (NRR:386). In a fixture: your `Fp2Budget` (T2:91-110). Read the
  flags `work_exhausted` and `time_exhausted` of the budget after each call that takes it: a set flag is a failure of
  the event (S1 10.5), not a refusal.
`Place` changes nothing. The same input gives the same output on each rank.

### 5.3 The overlay, and the life of the object
A rank does not find the binding of a new point or the trace of a new feature edge in the state, and `OwnedCell` does
not carry them (do not change `OwnedCell`). So `StateSurfaceGeometry` keeps an overlay over the state:
- `created`: map id -> `BoundaryPoint` (each new surface point of each rank of this call of `RunLocalCore`);
- `traces`: map edge -> `EdgeTrace` for the new child edges, and the set of the erased parent edges;
- the two key sets for the duplicate test.
`Accept(placement)` adds the point and its traces to the overlay of this rank and to the list "made in this pass".
The object holds a copy of the shared pointer `preparation.pending->state.original` that it was made from
(`RebuildFeatureTraces` takes this `PlanarReference`, and the copy keeps the old state alive at the install of section
8). It lives for one call of `RunLocalCore` (all passes), is not used after the install, and is destroyed before
`RunLocalCore` returns. It also holds the neighbour table of the reference facets for the walk (built one time).

## 6. The two surface steps in a pass (part A)

`RunPass` gets a last parameter `SurfaceGeometry* surface = nullptr` (both forms, hpp:203-204; the calls of FP1 do not
change). In `RunPass` (Pass:419-439), before `LengthWork`, when `options.surface_splits` and a geometry are given:
step L, then step D, then `MarkUnresolved()`. One time for a pass. (Reason, as a comment: the volume work never changes
a surface point, a surface face or a surface edge, so a second surface step in the same pass finds nothing.)
"Surface edge" = an edge of a `Physical` face. All orders are fixed; each tie-break uses the ids of points (S1 7).
Each step has its own set of refused edges and its own set of border edges (an edge whose `SurfaceRing` gave "border")
for the pass. An edge that step L refused can be split by step D.

Step L (length). As `SplitSweeps` (Pass:213-265) with these differences: candidates = the surface edges with
`Length > LongEdge` (feature edges take part) that are not in the two sets; the change is `SplitSurface` with
`progress_test`; no test `Blocked`. The same order (L descending, smaller id, larger id), the same stamp and deferral
rule, the same work list, the same limit of 50 sweeps. (`LongEdge` and `Mesh::Length` give the test of your prototype,
`MetricLength > 1.8`, SP:17-28.)

Step D (distance; your loop SP:231-266 with `MarkFeatures`, `MarkFaces` SP:401-448 and `ApplyMarks` SP:450-477). Keep
for each `Physical` face its four samples (a table of the step); compute them for a face that has none (at the start:
all; then: the children). Limit of 40 sweeps. One sweep:
1. Feature chords: for each feature edge that was not tried yet in this step: `Chord`. A refusal of `Chord` is kept
   as "unknown" (count it one time, `chord_unknown`): the edge is not tried again and is never marked. A feature edge
   with a chord above `Limit()` that is in none of the two sets is marked.
2. For each face whose largest sample is above `Limit()`, the first rule that fits:
   - one of its edges is a feature edge with a chord above `Limit()` (marked in 1, or in one of the two sets): nothing
     more for this face in this sweep (reason `refused` when that edge is in the refused set, `left_border` when it
     is in the border set, none when it is marked). This is intended and differs from your prototype (SP:409): no
     other edge is split for an error that belongs to a feature edge (the lesson of the folded root faces of FP2-0);
   - one of its edges is a feature edge whose midpoint sample is above `Limit()`, and its three other samples are at
     or below `Limit()`: the error belongs to the feature edge; mark no other edge (reason `feature_dominated`);
   - its longest edge (length in space) that is not a feature edge and not in the refused set: if this edge is in the
     border set, mark nothing (reason `left_border`); else mark it;
   - no such edge: mark nothing (reason `all_feature` when its three edges are feature edges, else `refused`).
3. Try `SplitSurface` (without `progress_test`) for the marked edges, longest first (length in space, then smaller id,
   larger id), with the stamp and deferral rule of step L. A refusal (each reason of 4.3 and 5.2) puts the edge in
   the refused set; the result "border" puts it in the border set; a deferred edge can be marked again.
Stop when a sweep marks no edge, or at 40 sweeps.
These are the rules of P0b 1.4 with these differences: the float fold check of P0b 1.5 is not used (4.3 step 7 is the
exact test); the samples and the limit come from the interface; the border set.
At the end of step D count the faces that are left above `Limit()`, by the reason of their last sweep (and "limit" at
the sweep limit). The engine does these.

## 7. The ranks (part B)

- A surface edge whose ring meets a `Border` face is not split in this pass (4.2). After the shift it can be local in
  pass 2. New surface points have all their cells on one rank when they are made.
- The exchange of the overlay, after EACH pass, before the shift (with more than one worker rank; with one rank there
  is no exchange): each rank sends what it made in this pass to all ranks: the points (`BoundaryPoint` rows), the new
  child traces (as fixed pages of words by the scheme of DI:1518-1555: `EncodeBoundaryAtlas` / `DecodeBoundaryAtlas`,
  BPh:35-38, with a page struct of your own, a copy of `AtlasPage`, DI:62-66; do not change DI) and the erased parent
  edges (an own record with `Fields`; they cannot go in the pages). Use `workers.exchange` with an admission, the
  pattern of Shift:31-41 (`World::metadata` has no admission). Each rank adds what it receives to its overlay; its own
  rows come back one time. The same id from two different ranks, or the same child edge from two different ranks: a
  failure of the event (it cannot happen with the ring rule and the id rule of S1 8; test it). After the exchange the
  overlays of all ranks are equal.
- `MeetBorderPoints` and `ShiftBorders` do not change.
- Each collective call of this item uses `trial.workers` only (S1 10.1) and runs on each worker rank, also on a rank
  that made nothing.

## 8. After the last pass: gather, build, check, install (part B)

Steps 1 to 4 are ONE function of the new library file (proposal: `BuildSurfaceState(World&, const Owned&, const
StateSurfaceGeometry&, size_t ceiling, size_t caller, ...)`, which returns the new state, or null when `splits_all`
is 0, and the peak bytes of its reservation). NRR computes `caller`, raises `stats.maximum_native_bytes`, and does
step 5.
`splits_all` = the sum over the worker ranks and the passes of this call of the accepted surface splits (one collective
sum). If it is 0: do nothing of this section (the state is not changed, its revision is not changed).
Else, on each worker rank:
1. Gather the faces: each rank sends, for each row of `working` and each k with `physical[k].present`, the
   `SurfaceFace{OrientedFace of face k as point ids, physical[k].facet}` to all ranks (an exchange with admission).
2. `SurfaceEdits` (the same on each rank): faces = the union of 1 (a face from two ranks: failure); points = for each
   vertex of these faces its `BoundaryPoint`, from `created`, else from the record of the state (`Find(id)`); traces =
   for each edge of these faces: the overlay trace, else the trace of the state when the edge is not erased. A point of
   `created` that no face uses: failure (this item makes no collapse).
3. Build, by the pattern of `Rebind` (BP:718-736), with `coverage = {traces, faces = {}}`:
   `live = caller + 256 * points + sizeof(SurfaceFace) * faces + 4 * BoundaryCoverageBytes(coverage)`;
   `bound = live + Authority()->ResidentBytes() + 2048 * points + 8192 * faces + 4 * BoundaryAtlasBytes({}, Mobility())`
   (BP:722-723); `reservation = BoundaryAtlasBudget(BoundaryAtlasBytes(coverage, Mobility()), ceiling, bound)`
   (BP:729); then `std::make_shared<BoundaryParameterState>(state.Authority(), points, faces, state.Revision() + 1,
   ceiling, live, coverage, state.Mobility(), &reservation, state.GeometryTolerance())` (BPh:88-91; BP:735).
   `ceiling = kernel.memory_ceiling`; `caller` (from NRR) = `kernel.caller_bytes + OwnedStorageBytes(working)` + the
   bytes of the geometry object; `BuildSurfaceState` adds the bytes of its gathered lists and uses the sum as the
   baseline of its exchanges and in `live`. Refuse `state.Revision() == UINT64_MAX` (BP:721).
   An exception of the constructor is a failure of the event (S1 10.5), with its message.
   Callable from the new file: `BoundaryCoverageBytes`, `BoundaryAtlasBytes`, `BoundaryAtlasBudget` (BPh:33-39),
   `Canonical` and `ResidentBytes` of the reference (ARh:281, :315). `FaceKey`, `FaceOrder`, `PointKey` and
   `BindingKey` are in the anonymous namespace of BP (BP:21-26): write own copies with the same rule (`FaceKey` = the
   sorted ids; `FaceOrder` = the smallest id rotated to the front, BP:24-26; the two point keys as your fixture has
   them).
4. Local check of the new state against `working` (the constructor does not see `working`): for each owned physical
   face: `next->Surface().at(FaceKey(face)).v == FaceOrder(face)` and `.facet ==
   Geometry().Canonical(physical[k].facet)`; each of its three points has a record with the same id and bit-equal
   coordinates. And the collective sum of the owned physical faces is `next->Surface().size()`. A difference: failure
   of the event.
5. Install (NRR): `preparation.pending->state.original =
   preparation.pending->state.original->WithParameterState(next)` (the pattern of `CNativeRemesher3D.cpp:103`).
   `RunLocalCore` gets `ReferencePreparation&` for this, and returns whether it installed a state. If it did,
   `AdaptWorker` computes `kernel.caller_bytes` again with the statement of NRR:685-687, directly after the call
   (NRR:703). (`RunLocalCore` cannot do it: the statement needs `input.imported`, and `kernel` is const there,
   NRR:779.) With the new option NO this statement does not run.
The engine then starts as today: `Directory::Build` compares the bytes of `Snapshot()` of all workers (DI:1463-1477);
the states are equal because the inputs of step 3 are equal.
A second call of `RunLocalCore` in the same event (normalization, `RunStages`, NRR:565-594) follows the same rules, on
the state that is in `pending->state.original` at that time (the result of the engine of the first stage, NRR:993).

## 9. Part C: the call, timers, memory, prints

- `RunLocalCore(mesh, trial, preparation)`: makes `StateSurfaceGeometry` (with its budget, 5.2) before the first pass
  when section 2 says so, gives it to each `RunPass`, runs the exchange of section 7 after each pass, and section 8
  after the last shift.
- Timers. All of this is inside the scopes of S1 10.2. Put the float samples, `Chord` and `Place` in
  `SU2NativeTiming3D::Scope(Operation::GeometryBound)` (an existing operation; the rows are then
  `construction / geometry_bound`). The policy build inside the constructor opens `MobilityPolicy` itself (AR:1464);
  `exchange` opens `Communication` itself. No new operation, no new phase, no new field: `fixedstorage=4120` stays
  (R10). No time without a name: do not use a bare `BcastRounds`.
- Memory. The admission of S1 10.3 also counts the bytes of the geometry object (the facet table, the overlay) and of
  the sample table of step D.
- Failure: S1 10.5 (elected on `workers`; no silent return to "engine only"). A refused split is not a failure.
- Prints, under `control.timings` only (R11). For each rank and pass:
  `NATIVE3D_LOCAL_SURFACE rank= stage= pass= applicable=1 faces_in= faces_out= points_in= points_out=
  splits_length= splits_distance= splits_feature= sweeps_length= sweeps_distance= left_long= left_border=
  left_distance= left_all_feature= left_refused= left_limit= feature_dominated= refused_walk= refused_protected=
  refused_feature= refused_trace= refused_duplicate= refused_outward= refused_size= refused_valid= refused_topology=
  refused_memory= refused_entity= chord_unknown= outward_max= samples= samples_unconverged= seconds_length=
  seconds_distance= seconds_samples= seconds_place= seconds_entity=`
  For each rank and call: `NATIVE3D_LOCAL_SURFACE_STATE rank= stage= splits_all= faces= points= traces= revision=
  seconds_exchange= seconds_gather= seconds_build= seconds_check= modeled_peak= rss_before= rss_after=`
  (`rss_*` = `CurrentRSS3D()`). No decision reads these values.

## 10. Fixtures and the lab (R3; new tags that G1 does not select)

Part A, one rank:
1. `Prepare` with `replaced`: accepted for a surface edge of a small box mesh with a fake geometry that gives the flat
   midpoint; refused when one parent only is listed. With an empty `replaced`: each fixture of FP1 passes without a
   change.
2. `SplitSurface` with a fake geometry that moves `m` inward and outward by a set distance: the cells are positive,
   the children carry the binding of their parent, `m` has `surface`, `Check()` passes, the outward guard refuses a
   step above tol and accepts one below, a refusal leaves the mesh bit-equal, `m` is not moved or removed by the later
   volume work of the pass, and the result with the work list on is the result with the work list off.
3. The two steps on a small curved case (a fake geometry: a sphere or a cylinder): step L and step D end, the counts
   are the same in two runs, each face left above the limit has a reason.
4. `StateSurfaceGeometry` on IN1 without a volume (a fake core is enough: your prototype loop with the production
   object): the constructor accepts the result; write the counts beside run A of FP2-0b.
5. LAB-S: the lab of FP1 (`[.][NativeLocalCoreLab]`, the input LAB1 = the real input of event 1 with its volume, one
   processor, variant V2) with the surface steps on. Keep the `ReferenceState` (today `LoadMesh` of the lab returns
   only the cells). The loaded reference has no policy and no entity geometry: do the setup of P0 section 1
   (`WithMobilityPolicy`, `WithEntityGeometry`; your `PrepareInput`), before or after `ImportGeometry`. Then: the pass
   with the surface steps; steps 2 to 4 of section 8 on one rank; then the two-sided proof of P0 row P4 on the new
   state (`OwnedFaceReverseBounds` and `GeometryCertificates(budget, false)`), and `Classify` for each face.
   LAB-S passes when: each cell is exactly positive; the constructor accepts; the check of 8.4 passes; the proof
   accepts each entity. Report: the row of section 9; the counts beside run A of FP2-0b (17,762 splits, 84,478 faces;
   they need not be equal: the volume rings, the progress test and the sweep limits differ); the count of faces with
   `Classify` above tol (expected 0); the cells and the bad cells at the end of the pass beside LAB1 V2 of FP1; the
   seconds of each part; `refused_valid` and `refused_outward`; `outward_max`.
Part B, MPI (2 and 4 ranks):
6. The exchange: a point and a feature trace that rank r makes in pass 1 are in the overlay of each rank before
   pass 2, and a rank that received the cells after the shift splits a child edge with them. A box volume with a fake
   geometry is enough for the point part.
7. Gather and build: the bytes of `Snapshot()` of the new state are equal on all ranks, and the check of 8.4 passes.
   No small reference with a volume exists today: use the LAB1 input at 2 and 4 ranks (a copy of the lab test without
   its one-rank test, `CNativeLocalCoreLab_tests.cpp:73`; test code, R3), or build a small case.

## 11. Gates

### 11.1 Parts A and B (each)
Build without a new warning. The fixtures of the part pass, and all fixtures of FP1. G1 identical. `git diff` shows no
call of the new code from NRR yet. Commit and push ("FP2-A part A", "FP2-A part B").

### 11.2 Part C with `ADAP_NATIVE_LOCAL_SURFACE= NO` (the default): an item with identical results
1. Build without a new warning; all fixtures; G1 identical.
2. G2 and GQ by the rule of section 2a of `AGENTS.md`, with `FP1/resume6/scheduling_calls_reference.json` for the calls
   of `construction / scheduling`. G3 (identity part). G4. The safeguards.
3. One G5 run with both options NO: the six files are the reference files (section 2a of `AGENTS.md`).
4. "Run CORE": one G5 run with the ON config of FP1 without a change (`R/night_20261115/FP1/run.cfg`, SHA256
   `1ec6c26d49d4ec7c62103d5bd096471b5422290cd06c44b2f1073b7489f2b4c1`; `ADAP_NATIVE_LOCAL_CORE= YES`, the new option
   absent): all standing gates; the six files are those of `R/night_20261115/FP1/on_reference.json`. This run is also
   the time reference of 11.3.
A failed gate here: R5; a revert takes part C only, parts A and B stay.
Commit and push ("FP2-A C: default proven") after 1 to 4.

### 11.3 Part C with both options YES: an item that changes the mesh
Config: a copy of `R/night_20261115/FP1/run.cfg` in `FP2_A/` with the one more line `ADAP_NATIVE_LOCAL_SURFACE= YES`;
write its SHA256. Your runner derives the config of the isolated restart from this copy, as it did for FP1. Two G5 runs ("run S1", "run S2") with the binary of 11.2 (no new build), as the next owned jobs after the run
CORE.
1. All standing gates in both runs (section 2 of `AGENTS.md`: 5400 s, 19073 MiB, the physical validator, transfer,
   continued CFD, isolated restart, 95 % timer coverage on each rank, join agreement, owned proof faces SUM).
2. The two runs give the same six files. They are then the reference for both options YES: write them with their
   SHA256 in `R/night_20261115/FP2_A/on_surface_reference.json`.
3. Time rule: the native maximum of event 1 of each run is not above that of the run CORE times 1.05. A miss of this
   rule alone is "OPEN for the user", not a failure.
4. The files are not compared with the files of the run CORE (the mesh changes; that is intended).
If a gate of 1 or 2 fails: ONE diagnosis pass (R5), the evidence and the first failing number in the report, mark
"FP2-A ON: FAILED", keep all code (the default is proven by 11.2), stop and wait for the user. No repeat by your own
decision.

### 11.4 Report rows for part C (run CORE beside S1 and S2, each event, each rank)
The native seconds; the phase seconds `preparation`, `construction`, `quality`, `final_proof`, `transfer`; inside
`construction` the rows `topology`, `geometry_bound`, `mobility_policy`, `communication`; the rows of section 9; the
surface splits that the ENGINE still makes after the core (its length splits and geometry splits on the surface) and
the faces that its first census finds above tol; cells, points and surface faces of the final meshes; the smallest
timer coverage; the job peak; how many times section 8 built a state in each event.

## 12. Report and close

`R/night_20261115/FP2_A/reports/FP2_A_<build>.md`: the source check (R4, one list), the gates of each part, LAB-S, the
rows of 11.4, each correction of test code (R3), each question (R13). Update the progress file, `HANDOFF_Codex.md`,
`HowToProceed.md` and `Handbook/ADAPTATION_TIMINGS.md`. Commit, push, verify that no owned job runs, stop and wait for
the user.

## 13. Not in this item

A collapse, a swap or a move of a surface point. The end of the wall-near rule (S1 6.2) and of the test `Blocked` for
the volume work. A contact test for a large outward step. A change of the engine, of `OwnedCell`, of DI, of the
constructor or of the mobility policy build (the policy build inside the constructor stays; FX1 part P is open). A
change of the tolerance, of the target, or a size from the curvature of the wall (parked by the user). The wall layers.
A new timer operation, phase or field. A change of a gate.
