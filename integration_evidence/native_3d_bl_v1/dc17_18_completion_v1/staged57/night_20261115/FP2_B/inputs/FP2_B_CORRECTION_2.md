# FP2-B correction 2: two rules for each surface split, and a lab on the production partition

Author: reviewer, 2026-11-22 (checked against the source and your evidence; the design was reviewed two times by a
second model). For the main agent. The user sends it; it is a user order. All parts of `FAST_PATH_STAGE2_B.md` ("the
text") and of `FP2_B_CORRECTION_1.md` ("correction 1") that this file does not change stay. Copy this file with its
SHA256 into `R/night_20261115/FP2_B/inputs/`.
Line numbers of `Surface.cpp` (`CNativeLocalCore3DSurface.cpp`), `Core.hpp`, `Run.inc` and `Lab` are those of your
commit `873eaf5a9e` ("D1": the FP2-B code of build633 with the print rows of D1).

## Why

Your run D1 was exact and it found the cause. The near-zero edges are made by step L (`SurfaceLength`), not by step D
and not by the engine. Event 2, rank 0, pass 1, facet 6036 (flat, y = 0): A = 87151, B = 22396, |A B| = 3.3249. Each
row has the face (A, X, B). Step L cuts A-P and B-Q in turn (2.21657 in space, 2.05 / 2.08 in the metric, long), and
each new point lands at the 1/3 or the 2/3 point of A-B; the distance to the point before it, the offset from the line
and the face area halve in each step (5.06e-3 ... 1.2e-9). A-B is not cut: it is in the `border` set of step L (its
other physical face is on the neighbouring rank; sweeps 1 and 2 have only `Border` refusals; event 1 shows the same
edges on ranks 2 and 3). This is the chain of correction 1 in step L. The reviewer wrote there that step L "does not
change (its chain ends when the edges are short in the metric)". That was wrong: the cut edges keep 2/3 of the locked
length.
The lab did not show it because the lab partition (unit weights, `Lab:726`) does not put a rank border through the
long region; the metric of N1 and N6 is the event-1 metric of S1.
The error that ended event 2 is "Degenerate metric donor" (`CNativeMetricTransport3D.hpp:32`: a donor cell with a zero
determinant in floating point; each donor of the batch is tested, so the donor can be far from the cell that the
message names). Which cell it is, is not known. A cell at such a chain is the reviewer's reading, not a fact.
Two rules follow, for BOTH surface steps, so that this kind of fault is closed by the model and not step by step:
- Rule 1 (order): a face is cut only through its longest edge. Step D has it (correction 1). Step L gets it here.
- Rule 2 (resolution): no accepted surface split makes a physical face with an altitude below `Limit()` in space. A
  face that thin is a line for the geometry test, and two surface points nearer than `Limit()` are one point for it.
  Rule 1 alone does not give this: a face that is already flat is cut correctly by rule 1 and still gets a new edge
  as short as its height, or a child that is thinner than itself.

## 1. Rule 1 in step L (production; `Mesh::SurfaceLength`, `Surface.cpp:408-481`)

In the loop over the candidates, when the turn of a candidate edge E comes and it passed the two deferral tests
(`Surface.cpp:448-458`), before the call of `SplitSurface` (`:460-461`), one small function (R9) that returns Split,
Wait or Locked:
for each physical face h of `SurfaceParents(E, ring)` (ring = the `EdgeRing` of E at `:453`; skip this test when
`ring.border`: `SplitSurface` refuses E as `Border` as now): let M_h be the longest edge of h by `Length`, compared as
`Ids(points_, .)` pairs of point ids; for equal lengths the smaller pair is the longer. Test all faces, then decide:
(1) if some M_h != E is LOCKED (it is in `refused`, `border` or `neighbor_locked` of this call of `SurfaceLength`, or
    `EdgeRing(M_h, h.cell).border`): do not split E; insert E into the new set `neighbor_locked` of this call. An edge
    in that set is not a candidate again in this pass (the filter at `:430`);
(2) else, if some M_h != E: do not split E in this sweep; `watch.Defer()`; `deferred.push_back(edge)` when the work
    list is on; count one in `waits_length` (M_h is longer than E in `Length`, so it is long; it was a candidate
    before E in this sweep and was deferred);
(3) else: call `SplitSurface` as now.
The stop rule of step L (`if (!accepted) break`) and its limit of sweeps do not change. An edge that still waits when
the step ends stays long; `SurfaceWork` counts it in `left_long` as now, and `MarkUnresolved` keeps the volume work
away from it as now.
`neighbor_locked` is not added to `SurfaceArrayBytes`: an edge is in at most one of the three sets, and the model
already counts two set entries for each edge. Write this in the report.
Comment at the function: rule 1 and its reason, and: without it step L cuts a face whose longest edge is locked
through its shorter edges; the cut edges keep 2/3 of the locked length, so they stay long, and the new points go to
the 1/3 and 2/3 points of the locked edge (D1: distances down to 1.2e-9).
Print (R11), in `Counts[]` directly after `path_max`: `left_long_neighbor=` (the size of `neighbor_locked` at the end
of step L, set inside `SurfaceLength`) and `waits_length=` (the deferrals of case (2), summed over the sweeps; not
the deferrals of the ring tests). Count a candidate that goes to `neighbor_locked` as `blocked` in the sweep row
(`++row.counters.blocked`), and set `evaluated_split = candidates - deferred - blocked` in `Emit`
(`Surface.cpp:104`); a wait of case (2) counts as `deferred`.

## 2. Rule 2 in `SplitSurface` (production; both steps)

In `Mesh::SplitSurface`, directly after the outward loop (`Surface.cpp:322-329`) and before `PointRow middle`
(`:331`), with the final coordinates of the new point m (`point.node.p`): for each of the two `parents` (`:300`), let
c be the vertex of `OrientedFace(cells_[parent.cell].vertices, parent.face)` that is neither `edge[0]` nor `edge[1]`
(NOT `vertices[parent.face]`, which is the apex of the cell). The split makes four physical child faces:
(edge[0], m, c) and (m, edge[1], c) for each parent. For each child compute its smallest altitude in space: twice its
area divided by its longest side. If one of the four values is not finite or is below `geometry.Limit()`: return
`refuse(SurfaceRefusal::Near)`.
Append `Near` to `SurfaceRefusal` (`Core.hpp:84-86`) and size `SurfaceSweepOutcomes` as `Near + 1` (`Core.hpp:101`;
without this the sweep row indexes out of bounds). Add `refused_near` to `SurfaceCounters`, to `Counts[]` after
`refused_entity`, to `PrintSurfaceSweepFields` (`Surface.cpp:168-175`), to `RefusalCount` (`:61-77`), and in test
code to `RankRefusalName` (`Lab:363-380`). The callers treat `Near` as each other refusal: E goes to the `refused`
set of the step, so it is LOCKED for rule 1 (step L) and for the path (step D).
Call no function of `geometry` other than `Limit()` for this test. Keep the exact positivity test, the entity test
and the floor `2 * Limit()` of step D as they are.
Comment: rule 2 and its reason (the lines of "Why"); and: with rule 1 the shape of a face does not go down without
limit, so a refusal `Near` comes only from a parent face that was already flat; such a face stays for the engine.
How many ordinary splits rule 2 refuses on N1 / N6 is not known (no print bounds the face altitudes; a refusal needs
a parent altitude below about 2 * `Limit()` = 1.4e-4); the lab reports `refused_near`.

## 3. The print rows that stay (print-only, as D1 made them)

Keep from D1, under the same switches and with the same rules (each print catches every exception):
- P1 `NATIVE3D_LOCAL_SHORT`: only the checkpoints `c1` and `shifted` (remove `c0` and `end`);
- P2 `NATIVE3D_LOCAL_NEAR` and `NATIVE3D_LOCAL_NEAR_LEFT`. Add one field `nearest_in_parents=` (1 when the nearest
  vertex is the third vertex of one of the two printed faces, else 0; the ends are never the nearest vertex,
  `Surface.cpp:252`). With rule 2 a row with
  trigger (i) and `nearest_in_parents=1` is a fault: write it in the report. Other rows are a report (trigger (i) also
  looks at the volume vertices of the star, which rule 2 does not test);
- P3 `NATIVE3D_METRIC_REJECT`;
- P4 `NATIVE3D_SEED_SHORT`.
The donor: do not change `CNativeMetricTransport3D.hpp:32` (the FP1 fixture `[NativeLocalCore3DResumeMove]`,
`CNativeLocalCore3DResumeMove_tests.cpp:38-39`, matches the exact text). In `EvaluateCavitySample`, wrap the call
`MetricAffineWeights(donors[i].cell, p)` (`:83`) in a try. Catch `std::invalid_argument& e` and rethrow
`std::invalid_argument` with the text `e.what()` followed by
` site=affine donor_cell=<id> donor_index=<i> donors=<size> v0=<id>@<x>,<y>,<z> ... v3=...` (setprecision 17). At
`:61` append the same fields with `site=orientation`. No other change in that file. R11 exception by this order: this
text is built at each throw, not under `options.timings`. No decision reads it (the only catch that continues,
`SampleMovedTensor`, `Changes.cpp:52-57`, catches by type and calls `MetricAffineWeights` directly).

## 4. Fixtures (serial; test code)

Add the friend entry `static void LengthStep(Mesh& m, SurfaceGeometry& g, IdSequence& ids, PassStats& r)` that calls
`m.SurfaceLength(g, ids, SIZE_MAX, r)` (do not name it `LengthWork`).
(e) The chain of D1, step L alone. Unit metric (`BoundMesh(..., 1)`), the points A (0,0,0), B (3,0,0), P (2,0.1,0),
    X (1,0.9,0), D (1.5,0.3,0.8), the tets (A,B,P,D) and (A,P,X,D). Only A-B (3.000) and A-P (2.0025) are longer
    than 1.8. The lock: remove the physical flag of the face (A,B,D), as `SkewedBox(true)` does, and build
    `Mesh(owned, true)`; then the ring of A-B has `border=true`. Use a geometry that records its `Place` calls
    (`LockedBoxGeometry::attempts`). Expected with the lock: sweep 1 refuses A-B as `Border`, and A-P goes to
    `neighbor_locked`: `splits_length=0`, `sweeps_length=1`, `left_long_neighbor=1`, `left_border=1`, no `Place` call
    for A-P, no new point; the long physical-face edges after the step, counted in test code: 2 (A-B, A-P).
    Expected without the lock: sweep 1 splits A-B first (the first `Place` is for A-B) and A-P is deferred by the
    ring test; sweep 2 splits A-P: `splits_length=2`, `sweeps_length=2`, `waits_length=0`.
(f) The wait branch (case (2) of rule 1) is reached only when a third, longer edge Z, split in the same sweep, stamps
    a cell of the ring of M that is not in the ring of E. If you build such a case (R3), expect `waits_length>=1` and
    the split of M, then of E, in later sweeps. Else write in the report that the branch is counted only in the lab
    runs and in S1.
(g) Rule 2. The fake geometries have `Limit()` = 0.014. Points A=0 (0,0,0), B=1 (3,0,0), C=2 (1.5,h,0),
    F=3 (1.5,-0.8,0), D=4 (1.5,0,0.8); tets (A,B,C,D) and (A,F,B,D); unit metric. A-B is the only long edge and the
    longest edge of both faces; `Place` gives M = (1.5,0,0). With h = 0.01: the split of A-B is refused `Near` in step
    L alone and in step D alone (the face (A,B,C) asks): `refused_near=1`, no new point, the point rows as before.
    With h = 0.05: the split is accepted.
(g2) The case of the second review, if it can be built inside R3, step D alone (the face (A,B,C) asks): in the
    plane, A (0,0), B (4,0), C (1, 1.25 d) with
    d = `Limit()`: each new edge is longer than d, and the child (M, B, C) has an altitude of about 0.83 d: refused
    `Near`. With C (1, 2 d): accepted. Else write that it was not built.
The fixtures of correction 1 ((a) to (d), (d) on the cylinder) and the A1 fixtures must pass without a change.

## 5. The order of the work

One whole-item source check (R4) before the first production edit; questions R13.
1. Bring the code back: `git checkout 873eaf5a9e -- Common SU2_CFD UnitTests`. Check that
   `git diff --stat 873eaf5a9e -- Common SU2_CFD UnitTests` is empty. Do not add untracked files (the two
   `CNativeDistributed3D.*.orig`). Label each commit of this item "default path not yet proven".
2. The lab on the PRODUCTION partition, FIRST on this code state (D1, without the two rules), so that the lab is
   shown to see the fault before it is used to show the cure. Test code; build `UnitTests/test_driver` on this state
   (no production change) and commit the test code with the label of item 1; best effort inside R3, at most
   90 minutes of your work for it (each run has the wrapper limit of 45 minutes); if it cannot be made, write why
   and go on with item 3.
   - N6P: the flow of N6, but with the partition that production makes for this input, in place of the unit weights:
     `DonorImport source(...)` and `source.Build(imported.original, ...)` (`Run.inc:469`, `:475`), `WorkingWeights(...)`
     with `PartitionWorkPhase::Coarsening` (`Run.inc:523-526`), `Repartition(..., geometry_weights)`
     (`Run.inc:538-541`). Keep `imported.original` in the lab input (`LoadMesh` drops it today); the composition is
     empty (`Run.inc:699`); the tolerance is `GetAdap_Hausd()`; give `reference.original` after `PrepareIN1` and a
     `BridgeStats`.
   - N7P: a new hidden case on the input of event 2, with its own pins (552,946 input cells; the markers of its
     `.native_ref`; do not change the event-1 pins of the rank check). Make its `sensor.bin` with
     `FP0/prepare_inputs.py:sensor()` from
     `R/night_20261115/FP2_B_D1/G5/build635/S1/full635_run1/restart_flow_adap_00001.dat` and
     `adapted_mesh_adap_00001.su2` (with `.native_ref` and `baseline_euler_seed.dat`). Its config is `FP2_A/run.cfg`
     with `MESH_FILENAME= adapted_mesh_adap_00001.su2` and `SOLUTION_FILENAME= restart_flow_adap_00001`; keep
     `ADAP_NATIVE_RANKS= 6`.
   - A run is VALID when, for each rank, the cells after the partition and `faces_in` of pass 1 equal the rows of S1
     (D1; event 1 equals build633). Cells after the partition, ranks 0 to 5: event 1
     48316 / 51155 / 56223 / 51202 / 50531 / 45932; event 2 95408 / 93521 / 94879 / 87449 / 86552 / 95137.
     `faces_in` of pass 1: event 1 9423 / 9031 / 6040 / 6694 / 9634 / 8132; event 2
     8490 / 15690 / 14277 / 12452 / 14459 / 10758 (sum 76126). Write the pairs.
   - Reproduction on the D1 code: a valid N6P must show, after the last shift, on rank 2: smallest metric edge
     3.907e-10 (64572-64650), 362 edges below 1e-2 and 138 below 1e-6; on rank 3: none below 1e-2 (the higher shift
     moved the rank-3 chain to rank 2; after pass 1, rank 3 has 5.45e-10). A valid N7P must show on rank 0: 1.617e-9
     (115118-115130), 23 below 1e-2, 10 below 1e-6. With the same code, partition and metric the values should be
     EQUAL to the rows P1 `shifted` / P4 of D1. Write each pair. If a valid run does not show them: write it; the lab
     then still misses something, and its later results are a report only.
3. Make the changes of sections 1 to 3 and the fixtures of section 4. Build `SU2_CFD` and `UnitTests/test_driver`
   without a new warning. The fixtures of section 4, of correction 1 and of A1; all fixtures of FP1; fixtures 1 to 4,
   6 and 7 of FP2-A section 10; G1 identical. Gate E does not repeat: the SHA256 of `CNativeLocalCore3DChanges.cpp`
   and `CNativeLocalCore3DPass.cpp` equals the pins of build633; the diff of `Core.hpp` and `Surface.cpp` against
   `873eaf5a9e` touches only step L, the test of rule 2 and their print fields. Commit and push ("FP2-B correction 2:
   fixtures passed; default path not yet proven"). If a fixture or the build fails after the one correction of R5:
   restore as item 7 says, mark "FP2-B correction 2: FAILED", commit the evidence, stop and wait.
4. The hand-over gate of section 5 of the text with the alarms of correction 1 section 3 item 2: N1, the invariant
   run on N1, and N6. (build633 step L: N1 2,416 splits in 7 sweeps; N6 2,416. The rules can change the numbers; write
   them beside build633 with `left_long_neighbor`, `waits_length`, `refused_near`, the faces left by reason.)
   One more condition of the gate, in N1 and N6, test code after the last shift (N1: after the pass): the DONOR SCAN:
   for each owned cell, `SU2Native3D::MetricAffineWeights(cell, Centroid(cell))` does not throw (count
   `std::invalid_argument`; print the first 5 cells with ids and coordinates). The gate asks for 0. (It covers the
   hand-over only; a donor that the engine makes later is not covered.) Run the same scan one time on the flow
   without surface splits (one more lab run, the case of H6off) and write its count beside (a report).
5. N6P and N7P again, with the rules. For a VALID run these conditions apply: items 1 and 2 of the hand-over gate
   (validity and proof; no edge below 1e-2), the donor scan, `v_limit=0`, `star_max` not above 1,000, and no pass
   above 600 s. The bad-cell count and the `faces_out` sums are a report beside S1 (event 1: 4,126 bad, `faces_out`
   73,962 / 76,940; event 2: 1,704 bad, 85,180 / 85,422). A valid run that fails a condition fails the gate. A run
   that is not valid is a report only.
   If the gate (item 4, or a valid run of item 5) fails: the rule of section 5 of the text (one diagnosis, no
   correction, no revert, commit the evidence, stop and wait).
6. If the gate passes: commit and push ("FP2-B correction 2: lab passed; default path not yet proven"), then part B of
   the text (section 6: run S1 first; then the default proof and S2 only if S1 passes; the failure paths as written
   there, with the revert of item 7 below in place of the revert wording of the text).
   For run S1 report, beside S1 of build633 and D1: for each event, stage and rank the rows P1 (`c1`, `shifted`) and
   P4 with their counts and the smallest edge; the counts of P2 rows by step, trigger and `nearest_in_parents`; the
   fields `left_long_neighbor`, `waits_length`, `refused_near`, `left_long`; each row P3 and each donor text. If S1
   fails: these are the first lines of the diagnosis; for a donor text, say if its four vertex ids are a cell of the
   hand-over (P1 / the lab) or were made by the engine.
7. The revert, where the text or this file asks for one: in ONE commit
   `git restore --source=bea11c37f7 --staged --worktree -- Common SU2_CFD UnitTests` ("FP2-B correction 2 reverted;
   production sources = build627"). Verify: `git diff --stat bea11c37f7 -- Common SU2_CFD UnitTests` empty; the
   build627 pins outside `UnitTests/`, 0 differences; `git diff --stat 852e566d17 -- Common SU2_CFD` empty; rebuilt
   `SU2_CFD` and `test_driver` with SHA256 equal to `binary_sha256` and `test_binary_sha256` of
   `R/night_20261115/FP2_B/build634/build_status.json`.

## 6. Not in this order

A surface collapse, swap or move (the next item). A change of step D beyond rule 2, of the V rounds, of the engine's
rules, of `Limit()`, of a limit of the core. Any other change of the text.
