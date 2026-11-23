# Fast path stage 2, item FP2-C part 2 step 0: measure the surface collapse before it is designed (test code only)

Author: reviewer, 2026-11-22 (checked against the source). For the main agent. The user sends it; it is a user order.
Copy this file with its SHA256 into `R/night_20261115/FP2_C2_0/inputs/`.
This item is a MEASUREMENT. Test code only (R3). No production change (R2): `git diff --stat <the last commit of
FP2-C part 1> -- Common SU2_CFD` stays empty; build only `UnitTests/test_driver`. No gate G1 to G5. No full run.
Start after FP2-C part 1 has its label ("S passed" or "<step> FAILED"); the code of part 1 stays. If the tree does
not build `UnitTests/test_driver`: do not start; write it; stop. If the lab of part 1 (its section 5 item 3) did not
pass: run this item with `wall_collapses` off; the sanity reference is then the S0 lab of part 1 (its section 5
item 1), compared on the fields that both runs print. Else the reference is the lab run 1 of part 1 of the same case.
Short names: `VH` = `UnitTests/SU2_CFD/adaptation/fixtures/native_volume_handover.inc`; `FA` =
`native_local_fixture_access.hpp`; `Changes`, `Surface`, `Mesh` = `CNativeLocalCore3D<name>.cpp`. Line numbers are
those of `c843610249`.

## 1. Why

The next production step is the collapse of a surface point in the core (remove a sheet point p of the surface onto a
neighbour k). Its design is not ready: a second review found that the reviewer's checks could accept a wrong surface.
Before the design is fixed, the reviewer needs numbers from the real surface: how many collapses each check would
accept, which check refuses most, what the checks cost, and how often another part of the surface is near. This item
tries each possible surface collapse "dry": it evaluates the checks and changes nothing.

## 2. Setup

Cases: N1 = `[NativeLocalCoreVolumeHandoverSerial]` (1 rank, one pass) and N6P =
`[NativeLocalCoreVolumeHandoverProduction1]` (6 ranks, production partition, two passes), in new hidden cases that
use the same flow with the production `RunPass` and the options of the lab of part 1. N6P uses 6 processes: no other
heavy process of ours at the same time (R1). Wrapper limit 60 minutes for each run.
Place: inside the pass of `RunVolumeHandover`, after `mesh.RunPass` and after the rows of the pass are printed, before
`mesh.WriteBack`. Add the seconds of the measurement in a pass to `measurement_seconds`, so that `seconds_pass`
stays comparable.
The measurement calls only these production functions: `Find`, `Samples`, `OnEntity`, `State()`, and through the
state `Geometry().MobilityDimension`, `Mobility()`, `GeometryTolerance()` and `EntityFaceError`; and the const
members of `Mesh` that the friend reaches. `Samples`, `OnEntity`, `Find` and `MobilityDimension` change nothing that
a later step reads, so the tests T1 to T11 and T13 run right after `RunPass` of each pass. `EntityFaceError` (T12)
fills a memo and a cache that the whole geometry shares; these change the `work` and `modeled_peak` of the later
proof. So T12 does NOT run in the pass: the measurement stores, for each child that T12 must evaluate, its facet,
its three bindings and its three coordinates (values, no reference into the mesh); T12 evaluates them on each rank
after the last row of the lab of part 1 has been printed (after the proof and the row `FP2_C cells`), on
`surface->State()`, with its own budget (`native_fp2_fixture::Budget(...)` as the proof makes it). Never call
`Chord`, `Place`, `Accept`, `Exchange` or `SetCallerBytes`. Catch an exception of one test of one direction in test
code and count it in a field `errors_<test>` (it is then a failure of that test).
The friend helpers that the measurement needs (star, ring, physical faces at a point, positivity, worst margin) are
new static members of `SurfaceFixtureAccess`, declared in `FA` and defined in `VH`. `Height` and `SmallestAltitude`
are file-local in `Surface` (`:143-153`): copy the two formulas into the test code.
Sanity: with `R/night_20261115/FP2_C1/compare_lab.py` and the row list of part 1 section 4 (the new rows of this
item excluded), every integer field equals the reference, except those whose name starts with `seconds` or contains
`rss`; this includes `work` and `modeled_peak` of the proof rows and the rows `FP2_C cells` (the measurement changed
nothing).

## 3. The candidates

The edges of the physical faces (`PhysicalFaces()`, each edge one time by its id pair) with
`Length(edge) < options.short_edge` (0.9). For each end p of such an edge, the DIRECTION (p removed, k = the other
end kept). Class of a direction, the first that applies: `border` (`Points()[p].border`); `corner` (the mobility
dimension of p is 0); `feature` (dimension 1); `sheet` (dimension 2). The mobility dimension of a point:
`state.Geometry().MobilityDimension(surface->Find(id)->binding, state.Mobility())` with
`state = surface->State()`. A `Find` that returns nullptr for a surface point that is not a border point is a fault
of the test code (stop the case).
On N6P a point's neighbours, skin neighbours and the points of T13 are those of this rank; for a border k the link
tests see only the cells of this rank: report the sheet directions with a border k separately.

## 4. The tests of a `sheet` direction

Notation: E = the edge (p, k). Star(p) = the live cells at p. Ring = the cells that hold both p and k. The physical
faces at p: for each cell c of Star(p) and each face f with `neighbors[f] == Physical` that holds p, the face
`SurfaceFaceAt({c, f})` (the ids in `OrientedFace` order and the facet of `Bindings(c)[f]`). A CHANGED face is one of
them without k, (p, a, b); its CHILD is that face with the id of p replaced by the id of k in place, with the facet
of the parent. The two faces with p and k go away. The new cells are the cells of Star(p) that do not hold k, with p
replaced by k (as `CollapsePlan`, `Changes:194-201`).
T1 is a precondition: a sheet direction that fails T1 is counted `ineligible` and gets no other test. Every other
test is evaluated for every sheet direction that passes T1, not only up to the first failure, with one exception:
T12 / T12L (see "Cost").
T1  two physical faces on E: `SurfaceParents(E, Ring).size() == 2`. Their third points are c1 and c2.
T2  link, points: N(p) ∩ N(k) equals the points of the Ring cells without p and k (N(x) = the points of the cells
    of Star(x) without x).
T2s link, skin: the points that are in a physical face with p and in a physical face with k are exactly c1 and c2.
T2e link, edges: each pair (x, y), x and y not p or k, such that a live cell holds p, x, y and a live cell holds
    k, x, y, is an edge of a Ring cell.
T3  no child exists already: no cell of Star(k) holds both a and b of a child (k, a, b).
T4  each new cell is positive in floating point: `TrialPositive` (the float test of `Prepare`). Also count, as a
    report, the directions that pass T4 but fail the exact `Positive` on a new cell.
T5  size, as `CollapsePlan` (`Changes:206-213`): the largest `Length({k, x})` over the points x of the new cells
    that are not a point of a Ring cell is not above `LongEdge`. Keep this value ("the longest new edge").
T5f feature chord: no edge (k, x) of a child that is not already an edge of the mesh has both ends of mobility
    dimension below 2.
T6  shape, the rule of `CollapsePlan` (`Changes:215-218`): refuse when the smallest `Margin` of the new cells is
    below `BadMargin` AND below `Worst(Star(p))`.
T7  altitude: the smallest altitude in space of each child is not below `Limit()`.
T8  reverse, witnesses: each of these points of the OLD surface at p has a distance in space to the NEAREST child
    (`SU2NativeCurved::detail::ClosestOnTriangle` over all children) not above `Limit()`: p; the midpoint of each
    spoke (p, x), x a vertex of a physical face at p; the centroid of each physical face at p (also the two faces
    on E). T8m (the weaker rule, for comparison only): p only.
T9  outward: h = `Height(child, p)` (n = (b-a) x (c-a) of the child in its oriented order; h = -n.(p-a)/|n|;
    h > 0 means that the new face is outside the old surface) is not above the tolerance, for each child. Keep the
    largest h. T9c: each child has h <= 0 (the new faces are inside the old cells).
T10 samples: `surface->Samples(SurfaceFace{{k, a, b}, facet})` for each child: the largest sample is not above
    `Limit()`. Report the count of samples that did not converge.
T11 entity: `surface->OnEntity(child, *surface->Find(child.v[0]))` accepts each child.
T12 the strict bound of the proof, as `ClassifyLabSurface` computes it for a face
    (`state.EntityFaceError(facet, the three bindings, the three coordinates, &budget,
    SU2NativeGeometry::SurfaceQueryNeed::Classify())`): not above the tolerance for each child. T12L: the returned
    bound is not above `Limit()` (one-sided: the bound can be loose below the tolerance; write that).
T13 the contact question: d_other(p) = the smallest distance in space from p to a surface point of this rank that is
    not in a physical face at p or at a skin neighbour of p (brute force; one time for each p, it does not depend
    on k).
Cost. T1 to T11 and T13 for every sheet direction that passes T1. T12 / T12L only for (a) the directions that pass
T1 to T7 and T9 to T11 (T8 and T8m not required), and (b) every 10th sheet direction in the sorted order (`Length`
ascending, then the id pair) for the "alone" counts. T12 runs after the proof (section 2), in that sorted order. If
the T12 time of one rank passes 25 minutes, it stops there; the report gives the share that was evaluated and the
counts so far. Time: the seconds of T10, T11 and T12 separately, and of all other tests together.

## 5. What to report (for each case, pass and rank, and summed)

R1. Section 3: edges, directions by class, the class of k for the sheet directions, the count `ineligible`.
R2. For each test: the count of sheet directions (that pass T1) that pass it alone; and the count that pass all
    tests up to and including it in the order T1, T2, T2s, T2e, T3, T4, T5, T5f, T6, T7, T8, T9, T10, T11, T12 (the
    chain). T8m, T9c, T12L and T13 are measurements beside the chain. The "alone" counts of T12 and T12L are from
    the 1-in-10 sample.
R3. The count of directions that pass every test of the chain except T8; and the same count when T8m is used in
    place of T8: how much the stronger reverse rule costs.
R4. Among the directions that pass the whole chain: how many have T9c true, and the histogram of the largest
    h / tolerance (bins: below 0; [0, 0.1); [0.1, 0.5); [0.5, 1]).
R5. Among the directions that pass the whole chain: the histogram of d_other / tolerance (bins: below 1; [1, 2);
    [2, 10); [10, 100); 100 and above; a rank with no other point writes `inf`) and the 20 smallest with p, its
    coordinates, and the id and coordinates of the other point.
R6. An estimate of one sweep: go through the edges that have a direction which passes the whole chain, in the order
    `Length` ascending then the id pair; take the direction with the smaller longest new edge (equal: remove the
    larger id; if only one direction passes, take it); accept it when no cell of its Star(p) (the cells before any
    change) is in the Star(p') of a direction accepted before in this estimate. Count the accepted ones (the points
    that one sweep would remove) and the faces that go away (2 for each). This is an estimate only.
R7. For the directions that fail the chain: the first failing test in the cost order T1, T2, T2s, T2e, T3, T7, T8,
    T9, T4, T5, T5f, T6, T10, T11, T12 (the counts by test).
R8. The seconds of T10, T11, T12 and of the rest, and the count of calls of each.
R9. Beside these: the surface faces and points of the case at the hand-over, and from the run S of part 1, event 1:
    (the surface faces at the engine's seed - `working_faces` at `length_collapse_exit`) / 2 = the surface points
    that the engine removes in its collapse pass (build640 S1: 86,254 -> 80,540, so 2,857; use this value if part 1
    has no run S).

## 6. Close

`R/night_20261115/FP2_C2_0/reports/FP2_C2_0_<build>.md` with the tables of section 5 and three lines at the top: the
sheet directions that pass the whole chain (N1: pass 1; N6P: pass 2, summed over the ranks); the estimate R6; the
count of R3. Update the progress file, `HANDOFF_Codex.md` and `HowToProceed.md`. Commit ("FP2-C2 step 0: measure the
surface collapse; test code only; no full run"), push, verify that no owned job runs, stop and wait for the user.

## 7. Not in this item

A production change of any kind. A collapse that is really made. A full run. A run with more than two adaptation
events (the runner and the controller assume two).
