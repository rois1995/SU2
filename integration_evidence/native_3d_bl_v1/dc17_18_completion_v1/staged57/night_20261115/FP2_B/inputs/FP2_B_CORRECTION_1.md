# FP2-B correction 1: step D cuts an edge only when it is the longest edge of each face at it (the longest-edge path)

Author: reviewer, 2026-11-22 (checked against the source and your evidence). For the main agent. The user sends it; it
is a user order. It replaces Rule A and Rule B of `FAST_PATH_STAGE2_B.md` ("the text"), section 4. All other parts of
the text stay. Copy this file with its SHA256 into `R/night_20261115/FP2_B/inputs/`.
Line numbers of `Surface` (`CNativeLocalCore3DSurface.cpp`) are those of commit `535d311882`.

## Why

Your lab stop was correct, and your code does what the text says (the reviewer's check found no wiring fault in A2).
The fault is in the rule of the reviewer.
Rule A stops a cut beside a face whose longest edge is LOCKED. A chain also runs beside a face whose longest edge is
free but is never cut, because no face asks for it as its own edge (both faces are below `Limit()`, or the face selects
a feature edge first). Its neighbour asks again and again for the shared edge, which is a SHORTER edge of the first
face. Each cut leaves the first face with the same longest edge and halves the cut edge toward one end (N6: four
edges with metric lengths 6.0e-3, 3.0e-3, 1.5e-3, 7.6e-4 on rank 4, at the wing tip near (1.089, 1.202, 0.004); this
is not the place of the chain of H6: Rule A stopped that one; the three leading-edge edges of N6 are not traced).
The rule that closes both cases is the longest-edge rule of bisection: a face is cut only through its own longest
edge. When the neighbour's longest edge is another one, that edge is cut first. For plane faces the smallest angle
then cannot go down without limit (Rivara); with the placed point on the model this holds up to its small offset from
the chord. A chain cannot form.

## 1. The change (production; step D only)

Definitions (as section 4 of the text): LOCKED (the two sets and the ring test), the other face at an edge, the longest
edge of a face. The floor `2 * Limit()` stays where it is (`Surface:451`, `:469`, `:509`).

The longest-edge path of a face f whose selected edge E passed the tests refused / border / short (`Surface:449-451`)
and is the longest edge of f (a function of its own, R9):
  e = E; g = f; steps = 0;
  repeat: let P = `SurfaceParents(e, EdgeRing(e, g.cell))`; take the first face h of P (in the order of P) with
          h != g and longest edge of h != e (compare in `ById` form as `LockedLongestEdge`, `Surface:430-431`);
          if there is no such h: stop; e is the TERMINAL edge;
          else: g = h; e = the longest edge of h; ++steps; if e is LOCKED: stop; the path is LOCKED.
  (e grows strictly in length, ties by the ids, so it ends; guard: throw when steps reaches the count of physical
  faces of the rank.)
Reason for the stop at a LOCKED edge: under this rule a face is split only through its longest edge. A face whose
longest edge is LOCKED does not change in this step, so E can never become the longest edge of each face at it.
An edge below the floor cannot occur on the path (E passed the floor and the lengths grow); write no test for it.
The decision for f (item 2 of step D, in the branch where E is the longest edge of the face; in place of Rule A):
- the path is LOCKED: mark nothing for f in this sweep; reason `neighbor_locked`;
- else the terminal edge is E: mark E as now (a ring border of E is found by `SplitSurface` as now: refusal `Border`,
  reason `Border`);
- else mark the terminal edge (it is not LOCKED). f keeps no mark of its own in this sweep: set its reason to `Limit`
  in this sweep (no new value of `SurfaceRemaining`); print-only count `waits_neighbor=` of such faces, summed over the
  sweeps. f asks again in the next sweep if it is still above `Limit()`. Such a face counts in `left_limit=` if the
  step ends at its limit of sweeps; at any other end of the step no face waits (that sweep marked an edge).
Item 1 of step D (a feature edge F with a chord above `Limit()` that passes the tests of item 1; in place of Rule B):
for each physical face h of `SurfaceParents(F, EdgeRing(F, use.cell))` whose longest edge is not F, follow the path
from (e = the longest edge of h, g = h), with the LOCKED test at its first edge too. If any of these paths is LOCKED:
mark nothing for F in this sweep (as Rule B). Else, if no face needed a path: mark F. Else mark the terminal edges of
these paths and not F.
In item 2, a face whose selected feature edge F is not in `marks` gets the reason `Limit` when item 1 marked a terminal
edge for F in this sweep, else `neighbor_locked`; it is not added to `chosen` (the guard for `marks.at` stays).
Such a face (reason `Limit`) also counts in `waits_neighbor=`. Keep these F of the sweep in a local
`std::set<SurfaceEdge>` cleared at each sweep (at most the feature edges of the rank; it is not in
`SurfaceArrayBytes`; write that in the report). "No other container" in the print line below applies to the
`path_splits` flag only.
A marked edge that is not the edge of an asking face is split by `SplitSurface` as each other marked edge (placement
by its own kind: sheet or feature; its refusals go to `refused` / `border` as now).
The stop rule of step D does not change (a sweep that marks nothing ends the step). The limit of 40 sweeps stays.
`left_neighbor_locked=` keeps its meaning (faces above `Limit()` at the end with that reason). Step L does not change.
Comment at the function: the rule and its reason, and: each accepted split of step D is a split of an edge that is
the longest edge of each physical face at it (the code defers a marked edge whose ring changed in the same sweep).
Print (R11), appended to `Counts[]` after `left_neighbor_locked`: `waits_neighbor=`; `path_splits=` (accepted splits
of step D of an edge that, in its sweep, was neither the E of a face whose path ended at E nor a feature edge F
marked by item 1; keep this flag in `DistanceMark`, no other container); `path_max=` (the largest `steps` of a path
in the pass; 0 when the terminal edge is E).

## 2. Fixtures (serial)

They replace the A2 cases (`CNativeLocalCore3DSurface_tests.cpp:516-571`) and the helper
`SurfaceFixtureAccess::DistanceChoice` (it now returns the edge to mark, or the reason). Do not keep their old
assertions.
(a) Step D alone (`SurfaceFixtureAccess::DistanceWork`, as the A2 cases) in `SkewedBox(false)` with a geometry where
    only f = (0,1,2) is above the limit: the path of f is 0-2 -> 2-3 -> 3-6. Sweep 1 splits 3-6, sweep 2 splits 2-3,
    sweep 3 splits E = 0-2; `path_splits=2`, `waits_neighbor=2`, `path_max=2`, `sweeps_distance=3`,
    `left_neighbor_locked=0`, no face left. No edge of the result is shorter than half of the shortest edge of the
    input (equal is permitted). These values follow from the coordinates (|2-3| = 2, |3-6| = sqrt 5,
    |3-7| = sqrt 2, |0-2| = sqrt 2). Another order is a failed check (R5), unless its cause is in your test code
    (geometry or harness, R3); do not pin another order.
(b) The same with the path LOCKED. Run (the box with the `Border` face: the ring of the longest edge of g meets it;
    only f above): f marks nothing, `left_neighbor_locked=1`, no `Place` call, `sweeps_distance=0`. Helper
    (`SkewedBox(false)`, refused = {2-3}): it returns `neighbor_locked` for f, not 3-6.
(c) `SkewedBox(false)` with `LockedBoxGeometry(rows, true)` (both faces above) and the feature edge 0-2 (chord above
    the limit): sweep 1 splits 3-6, sweep 2 splits 2-3, sweep 3 splits 0-2; `path_splits=2`; both faces have the
    reason `Limit` in sweeps 1 and 2; the pass ends without an exception; `chord_queries == 1`.
(d) Invariant, test code: a forwarding `SurfaceGeometry` wrapper passed to the PRODUCTION `RunPass` as its surface
    pointer. A split belongs to step D when the wrapper has seen a `Samples` call in this pass (only
    `SurfaceDistance` calls `Samples`). In `Place` (before the mesh changes), for each of the two faces passed: is the
    edge its longest edge, in space (`std::hypot` of the coordinate differences, as `Surface:124`), ties by the
    smaller sorted pair of ids? Take the third vertex from `mesh.Points()` by id (the cylinder geometry has no
    `Find`). Count a violation when the next call is `Accept` and one answer was no. On the cylinder fixture of
    FP2-A, and on N1 in a SEPARATE hidden case. The gate run N1 of section 3 runs without the wrapper, as section 5 of
    the text says. The wrapper run must give the same integer fields of `NATIVE3D_LOCAL_SURFACE` and the same
    `bad_out` as the gate run (else it is a fault of test code, R3). Violations: 0 in both. Use one wrapper for each
    call of `RunPass`; "the next call" means the next call of `Place` or `Accept`. Violations above 0 on N1 are a
    failure of the hand-over gate (section 3 item 4).

## 3. What to repeat, and the order of the work

One whole-item source check (R4) before the first production edit; questions R13.
1. Build `SU2_CFD` and `UnitTests/test_driver` without a new warning. The fixtures of section 2 ((d) on the
   cylinder; (d) on N1 comes with item 2), the A1 fixtures, all fixtures of FP1, fixtures 1 to 4, 6 and 7 of FP2-A
   section 10 (the cylinder sum as corrected), G1 identical. Gate E does not repeat: write that the SHA256 of
   `CNativeLocalCore3DChanges.cpp` and `CNativeLocalCore3DPass.cpp` equals that of the `source_pins.json` of build632,
   and that the diff of `Core.hpp` and `Surface` since `535d311882` touches only step D and its print fields.
   Commit and push (R8): "FP2-B correction 1: fixtures passed; default path not yet proven". If a fixture of
   section 2 or the build fails after the one correction of R5: revert this correction only (A1 and A2 stay), mark
   "FP2-B correction 1: FAILED", commit the evidence, stop and wait.
2. The hand-over gate of section 5 of the text: N1 and N6 again, the same conditions, and fixture (d) on N1. One more
   alarm (the path cuts faces that did not ask): the sum of `faces_out` over the ranks is not above 95,413 (N1) and
   95,305 / 95,422 (N6, pass 1 / pass 2; 1.15 times 82,968 / 82,874 / 82,976 of build632). In the report beside
   N1 / N6 of build632: faces, splits by kind, `path_splits`, `waits_neighbor`, `path_max`, `sweeps_distance` of each
   rank, bad cells by class, the edge counts and the smallest edge, the census line.
3. If the gate passes: commit and push ("FP2-B correction 1: lab passed; default path not yet proven"), then part B of
   the text (section 6) without a change.
4. If the gate fails again: the rule of section 5 of the text (one diagnosis, no correction, no revert, commit the
   evidence, stop and wait). For a failure of the edge condition (item 2 of the gate) the diagnosis must give, for each
   edge below 1e-2, the split that made each of its two ends, from a second run of N6 with the PRODUCTION `RunPass`
   and the wrapper of (d), extended to print for each accepted split: step (L / D), pass, sweep (from the
   `stats.sink` rows of that pass, as B0 did), the split edge, the three ids of each of the two faces, and the
   longest-edge answer of each face. Not with `HandoverPass` (it has no V rounds, so its pass 2 differs). The run must
   reproduce the gate run: the same `faces_out` and `splits_distance` of each rank and pass, and the same 3.3 edge
   rows. If an end of a short edge is an input point, write "input".

## 4. Not in this order

A change of step L, of the V rounds, of `Limit()`, of the floor, of a limit of the core. Any other change of the text.
