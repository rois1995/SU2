# FP2-B correction 3: the sheet placement accepts a foot on the model by the rule of the engine's projector

Author: reviewer, 2026-11-22 (checked against the source and your evidence; the design was reviewed by a second
model). For the main agent. The user sends it; it is a user order. All parts of `FAST_PATH_STAGE2_B.md` ("the text"),
`FP2_B_CORRECTION_1.md` and `FP2_B_CORRECTION_2.md` that this file does not change stay. Copy this file with its SHA256
into `R/night_20261115/FP2_B/inputs/`.
Line numbers are those of your commit `da8088eae2` (build638). Short names: `SG` =
`CNativeLocalSurfaceGeometry3D.cpp`, `CS` = `CNativeLocalCore3DSurface.cpp`, `GB` = `CNativeGeometryBackend3D.cpp`
(`Project`), `BP` = `CNativeBoundaryParameters3D.cpp`, `CR` = the file of `MinimizeTriangle`.

## Why

Your run S1 of build638 passed both events and the restart, and the two rules did what they should: no edge below
1e-2 at the hand-over, no donor error, and the quality phase of the engine in event 1 needs 216 s (build633: 1,112 s;
run CORE: 829 s) because it starts with 2,999 / 2,963 failing cells in place of about 13,000.
Event 1 still needs 4,109 s. The time is in the geometry repair of the engine: 1,488 cells at its entry (build633: 84).
In build633 and in run CORE the engine cut these faces in extra construction passes that the stuck edges of the step-L
chain kept alive; correction 2 removed that chain, so these faces now reach the slow repair.
The faces are left by the core: 1,365 faces above the tolerance at the hand-over (N6P); in S1, pass 2:
`left_neighbor_locked` 1,811, `left_refused` 272. At least 96 % of the refused edges are `Walk` refusals (193 in
pass 1, 238 in pass 2; N1: 236), and each locks about 7 more faces by the two rules.
The walk refusals are not hard geometry. The query (the chord midpoint) is on the model to about 1e-11 and its foot
lies on an edge of a reference facet. `MinimizeTriangle` misses its stationarity test there by roundoff, enters the
interior again, and returns false after its 6 outer loops (`CR:481`). The projector of the engine uses the same
minimiser and has one more rule: it takes a foot that did not converge when its distance to the query plus its error
bound is not above tol / 8 (`GB:176-181`: "A valid model foot within the requested spatial accuracy needs no further
optimality proof"; `BP:1444` sets that accuracy). The walk of the core does not have this rule. This correction gives
it the same rule. (The reviewer rebuilt the walk outside the solver on the event-1 reference: 0 refusals in sweep 1
and 151 in sweep 2, production 0 / 140; 148 of the 151 points are within 4e-11 of their query; 3 had no start facet
in that rebuild.)
Reason of the rule: for a valid binding b, `|q - Realize(b)| + ErrorBound(b, Realize(b))` bounds the distance from the
query to a model point. If it is not above tol / 8, a perfect closest-point search could not be nearer by more than
tol / 8. That is enough for the proposal of a split point; the strict tests that follow certify the point.

## 1. The change (production; `StateSurfaceGeometry::Impl::PlaceOnSheet`, `SG:584-601`)

Replace line `SG:591` (`if (!hit.converged) return Refused(SurfaceRefusal::Walk);`) by a test for a walk that did not
converge. The converged path (`SG:592-597`) does not change.
- If `hit.facet == UINT64_MAX` (no start facet): refuse `Walk` as now.
- Else, if `!hit.converged`: let `b = walker.Binding(hit)`, `p = state.Geometry().Realize(b)`,
  `d = std::hypot((long double)query.x - p.x, (long double)query.y - p.y, (long double)query.z - p.z)` and
  `upper = std::nextafter(double(d + state.Geometry().ErrorBound(b, p)), INFINITY)` (the formula of `GB:177-179`).
  If `upper` is not finite or is above `state.GeometryTolerance() / 8`: refuse `Walk`. Else continue at `SG:592`
  exactly as a converged walk (the mobility test, `Realize`, `Duplicate`).
- An exception anywhere in `PlaceOnSheet` is caught at `SG:598-599` as now.
Put the bound in one small function (R9), for example
`double ProjectorBound(Point query, const SurfaceBinding& b) const`, which returns `upper`. The caller refuses `Walk`
when it is not finite or is above `state.GeometryTolerance() / 8`.
The tests of `SplitSurface` that follow (`CS:331-414`: the outward guard, rule 2, the size test, `Prepare` with the
exact positivity, the topology test, `OnEntity` for the four children), `Accept`, and at the end `BuildSurfaceState`
and the check of the owned faces do not change and stay in their order.
Do not change `MinimizeTriangle`, the walker's hops, `Samples` (a sample that did not converge is an upper bound of the
distance; no decision reads its flag), the feature placement, or a file of the engine.
Comment at the function: the rule and its reason (the last paragraph of "Why"; the same accuracy rule as the projector
of the engine, `GB:176-181` and `BP:1444`).
Print (R11). Add to `SurfacePlacement` (`CNativeLocalSurface3D.hpp:27-34`) one print-only field
`SurfaceWalk walk = SurfaceWalk::None;` with `enum class SurfaceWalk { None, Tolerance, Far, NoStart, Exception };`.
`PlaceOnSheet` sets it: `Tolerance` when the rule of this section passed, whatever the later tests give; `Far` when
`upper` is above tol / 8; `Exception` when `upper` is not finite, and in the catch at `SG:598-599` (an exception
later in `PlaceOnSheet` sets `Exception` also after `Tolerance` was set); `NoStart` for `hit.facet == UINT64_MAX`.
No decision reads it.
Add `walk_tol8`, `walk_far`, `walk_no_start` and `walk_exception` to `SurfaceCounters` (`Core.hpp:127`, after
`refused_near`) and to `Counts[]` directly after `refused_near` (`CS:57`). In `Mesh::SplitSurface`, directly after
`geometry.Place` (`CS:320-321`), `if (report)` add one to the counter of `placement.walk` (none for `None`).
`walk_tol8` counts placements where the rule passed; some of them can still be refused by a later test. For each row
`walk_far + walk_no_start + walk_exception == refused_walk`: write this check in the report.

## 2. Fixtures (serial; test code)

(h) On IN1: a new hidden case beside `[.][NativeFastPathStage2Surface]` with the same `PrepareInput` and
    `local::StateSurfaceGeometry surface(reference.original, budget)`. For each physical edge of `state.Surface()`
    with two faces and no trace (`!surface.Feature(edge)`), in the order of the sorted edge ids, call
    `surface.Place({a, b}, {f0, f1}, id)` (a and b are the `node`s of `surface.Find(.)`, f0 and f1 the two faces of
    the state, id one fresh id above all ids, the same for each call; `Place` does not publish). Take the first three
    placements with `walk == SurfaceWalk::Tolerance` and `Accepted()`. For each, check in test code:
    - p (`placement.point.node.p`) equals `surface.State().Geometry().Realize(b)` bit for bit, with b =
      `placement.point.binding`;
    - `std::nextafter(double(d + Geometry().ErrorBound(b, p)), INFINITY) <= surface.Tolerance() / 8`, with d the
      distance in `long double` from q to p, q = `0.5 * (a + b)` for each coordinate in double (as `Midpoint`,
      `SG:291-293`);
    - `MobilityDimension(b, State().Mobility()) == 2`;
    - `surface.OnEntity(child, placement.point)` is true for the four children (the two faces with each end replaced
      by id, as `native_fast_path_stage2_split.inc:536-544`).
    Print the counts by `walk` over all edges and the three edges (ids, q, p, binding). If a placement has
    `walk == SurfaceWalk::Far`: check that it is refused `Walk`, and print it. If no edge gives `Tolerance`: write it;
    the evidence is then the counts of section 3 item 3.
(i) In the case of (h): the first three placements with `walk == SurfaceWalk::None` and `Accepted()` give the same
    binding (dimension, feature, u, v, version) and the same coordinates (bit for bit) as on build638. Pin these
    values from one run of the same loop on the tree of `da8088eae2` BEFORE the production edit (build only
    `test_driver`, test code only, as correction 2 item 2 did; without the field `walk`, take there the first three
    accepted placements whose walk converged). Write them in the fixture and the report. Their `walk` is
    `None` (so no new counter would move). In the same pre-edit run, also write the ids of the edges whose placement
    build638 refused `Walk`; the three `Tolerance` edges of (h) must be among them.
(j) In the invariant run on N1 (`[.][NativeLocalCoreVolumeHandoverInvariant]`), the probe `SurfaceLongestEdgeProbe`
    (`native_local_surface_probe.hpp`) keeps the `walk` and the edge of the last placement. It counts the placements
    with `walk == Tolerance`: (a) those that `Place` refused (by `refusal`: Protected, Duplicate); (b) those that
    reach `Accept`; (c) the rest, which `SplitSurface` refused later. For (c) the test sets
    `stats.surface_refusal_sink` (`CS:93-94`) to a function that gives the probe the edge and the refusal; when the
    edge is the probe's last placement, the probe counts that refusal (Outward, Near, Size, Validity, Topology,
    Entity). It prints a + b + c and each part. a + b + c must equal `walk_tol8` of the row, and b must be at least 1
    (a rescued placement that becomes a split). The row's integers and `bad_out` must equal those of N1 as before.
    The probe calls no geometry function itself.
The fixtures of corrections 1 and 2 and the A1 fixtures must pass without a change.

## 3. The order of the work

One whole-item source check (R4) before the first production edit; questions R13.
1. The default proof of correction 2 on build638 stops: if one of its checks runs, let it end (R12), and do not start
   its next check or S2. Its finished checks are a report and stay as evidence. Before any edit or build, verify that
   no owned job runs. The code state is the sources of `da8088eae2`. If the tree differs there (for example after the
   revert of a failed proof): `git restore --source=da8088eae2 --staged --worktree -- Common SU2_CFD UnitTests`, check
   that `git diff --stat da8088eae2 -- Common SU2_CFD UnitTests` is empty, and commit ("FP2-B correction 3: build638
   sources restored; default path not yet proven").
2. The pins of fixture (i) on this state (test code only). Then the change of section 1 and the fixtures of section 2.
   Build `SU2_CFD` and `UnitTests/test_driver` without a new warning. The fixtures of section 2 ((j) comes with
   item 3), of corrections 1 and 2, of A1; all fixtures of FP1; fixtures 1 to 4, 6 and 7 of FP2-A section 10; G1
   identical. Gate E does not repeat: the SHA256 of `CNativeLocalCore3DChanges.cpp` and `CNativeLocalCore3DPass.cpp`
   equals the pins of build633; the diff against `da8088eae2` touches only `PlaceOnSheet` and its new function, the
   field `walk` and its enum in `CNativeLocalSurface3D.hpp`, the four counters in `Core.hpp`, `Counts[]`, and the
   counting lines after `Place` in `SplitSurface`. Commit and push ("FP2-B correction 3: fixtures passed; default path
   not yet proven"). If a fixture or the build fails after the one correction of R5: go back to the tree of
   `da8088eae2` (the restore command of item 1; delete by name the new files that this correction added under these
   paths; no `git clean`), mark "FP2-B correction 3: FAILED", commit the evidence, stop and wait.
3. The lab: N1, the invariant run on N1 with (j), N6, and N6P.
   Conditions for N1 and N6 (the text section 5 with corrections 1 and 2): each cell exactly positive; the check of
   8.4; the proof condition (each entity accepted, or each face above tol has a reason; `classify_unexplained=0`);
   N6: the bytes of `Snapshot()` equal on all ranks; no core metric edge below 1e-2 at the end of the last pass;
   `star_max` not above 1,000; `sweeps_distance` below 40; the sum of `faces_out` not above 95,413 (N1) and
   95,305 / 95,422 (N6, pass 1 / pass 2); no pass above 600 s on a rank; `v_limit=0` on each rank and pass; donor
   scan 0; each run inside its 45-minute wrapper; the invariant run: violations 0, the same integers and `bad_out` as
   N1, and (j).
   N6P must be VALID (as on build638). If its partition does not match, write the first number that differs, commit
   ("FP2-B correction 3: N6P not valid; default path not yet proven"), do not start S1, stop and wait. Conditions for
   the valid N6P: the first five items above (positivity, the check of 8.4, the proof, the snapshots, no edge below
   1e-2), the donor scan, `v_limit=0`, `star_max` not above 1,000, no pass above 600 s. Its `faces_out` is a report.
   All runs: the bad cells are a report in this correction (the faces that are no longer locked give more splits and
   so more cells for the volume rounds); write the bad cells by class before and after the V rounds, the `v_changes`,
   and the splits by step, beside build638. One more condition (an alarm for a change that makes it worse, not a
   target): the field `classify_above_tol` of the row `FP2_B proof` is not above its value on build638: 1,204 (N1),
   1,203 (N6), 1,365 (N6P).
   Report beside build638, for each run, rank and pass: `refused_walk` and the four `walk_` counters with the check of
   their sum; each other refusal kind (by sweep where the sweep rows have it); `left_distance`, `left_refused`,
   `left_neighbor_locked`, `left_border` (it counts refused edges), `left_long`, `left_long_neighbor`; the faces above
   the tolerance by reason; the splits by kind, `faces_out`, `sweeps_distance`, `star_max`, the smallest edge, the
   donor scan, the seconds of the surface steps and of the placement.
   Two more measurements in test code (a report):
   - in `ClassifyLabSurface`, for each face with `bound > tol`, the bin of `bound / tol - 1`: below 0.01, below 0.1,
     below 1, below 10, else; print the bins on the proof row (build638 has only the totals);
   - in N6P, after the last shift, on each rank, in the mesh of the "shifted" report
     (`Mesh(input.cells, true, options)`, `native_volume_handover.inc:389-397`): for each physical face of an owned
     cell, take its longest edge in space (ties: the smaller pair of ids) and, through the friend
     `SurfaceFixtureAccess`, `EdgeRing(edge, cell).border` with that cell as the seed. Pack (the three sorted ids, the
     flag). Gather the rows on rank 0 as `GatherVolumeReasons` does (a key from two ranks is a test failure). In
     `ClassifyLabSurface`, for each face above tol, count by excess bin the faces whose longest edge has a ring
     without a rank border (a third surface pass could cut these) and those with a border. A face with no row is a
     test failure.
   Expected by the reviewer (a report): `refused_walk` near 0, N1 near 0 faces above the tolerance, N6P about 100 to
   300 (faces at rank borders).
   If the gate fails: the rule of section 5 of the text (one diagnosis, no correction, no revert, commit the evidence
   with the label "FP2-B correction 3: lab stopped before S1; default path not yet proven", stop and wait).
4. If the gate passes: commit and push ("FP2-B correction 3: lab passed; default path not yet proven"). Then run S1
   (the config of the text section 6 item 1) on this binary, with all standing gates of both events.
5. After S1, in each case: write the rows of the text section 6 item 4, and for event 1 beside S1 of build638: the
   engine seconds of construction, of the quality phase and of each geometry repair; the geometry-bad cells at
   `length_collapse_exit` and at the entry of the first repair, by rank and by bin; the P rows as correction 2 asks;
   the four `walk_` counters of each rank, pass and event. Commit and push the evidence ("FP2-B correction 3: S1
   passed; default path not yet proven" or "FP2-B correction 3: S1 FAILED; default path not yet proven"), verify that
   no owned job runs, STOP and wait for the user. Do NOT start the default proof or S2, and do not revert. (This user
   order replaces, for this correction, the revert of R5 and the automatic paths of the text section 6 items 2 and 3:
   the reviewer decides from the time of event 1 whether the default proof runs on this state or after one more
   step. Nothing is accepted before the default proof and S2.)

## 4. Not in this order

A strict projection as a second way to place a point (not necessary by the evidence; the counters `walk_far`,
`walk_no_start` and `walk_exception` show if it is). A third pass of the core or a change of the shift (the faces
at rank borders; the measurement of item 3 prepares that decision). A change of the engine (its construction exit
rule, its repair). A change of `MinimizeTriangle`. A surface collapse, swap or move. Any other change of the text.
