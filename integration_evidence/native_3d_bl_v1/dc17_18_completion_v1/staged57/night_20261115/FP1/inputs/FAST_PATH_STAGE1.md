# Fast path, stage 1 (item FP1): the local core for the Euler volume, surface frozen

Author: reviewer, 2026-11-19. For the main agent. Base: the accepted state `e40d7e3795` with the FP0 test code `689ae4746e`.
All rules of section 1 of `AGENTS.md` apply (R1 to R13). R = `integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57`.
Context (not an order): `FAST_PATH_DESIGN_V1.md` (your copy: `R/night_20261115/FP0/inputs/`). Evidence folder: `R/night_20261115/FP1/`.

Short names: NRR = `Common/src/adaptation/native_engine3d/NativeRemeshRun.inc`. "The engine" = the present 3D engine
(`AdaptMetric`). "The core" = the new code of this item. "ON" / "OFF" = the new option of section 2.1.

The item has three parts. Do them in the order A, B, C. Each part has its own gates (section 12).

| Part | Content | Production effect |
|---|---|---|
| A | The core as a library for one rank, with unit fixtures | none (no call from production) |
| B | The border shift between ranks, with MPI fixtures | none (no call from production) |
| C | The option, the call in `AdaptWorker`, the full gates | OFF: identical results. ON: a new mesh |

## 1. Why, and what this item is

1. The user decided the fast path on 2026-11-19. Stage 1 builds its base: flat arrays with a neighbour table, the five
   changes (collapse, split, remove an edge, swap a face, move a point), new numbers, borders between ranks, same result
   in two runs, all gates. Stage 2 (the surface in the core) uses all of it.
2. The core changes only the INTERIOR of the volume. Each surface point and each surface face stays bit-equal.
   After the core, the engine runs without a change. It is the checker and the repairer. All final gates stay.
3. The gain in time of this stage alone is not known and can be small. The purpose is the base. Section 12 has one time
   rule; a miss of that rule alone is "OPEN for the user".
4. Facts from the source that this design uses (source study of 2026-11-19, HEAD `a59380df18`):
   - In the Euler path the engine takes its target from the CURRENT nodal tensors of the working cells
     (`EngineAdapt.cpp:105`, `EngineSetup.cpp:226-228`, `FrozenField(..., current_nodal = true)` at `EngineCensus.cpp:247-248`).
     `stage_source` is a snapshot of the working cells. The original donors are not read by the worker when each row has
     `sensor_present` (NRR:725-726). So the core uses the same rule: the target is the nodal tensors of its own points.
   - The engine makes few collapses (2,495 in event 1 of the run `round_8_5t_P`; rank 0 has 37,819 short edges and accepts
     480). The cause is the organisation of its one collapse pass (`EngineSelectionQueues.cpp:662-689`: each commit drops all
     candidates near it; new edges are never candidates; `EngineControl.cpp:1342-1350`). It is not an acceptance rule. The
     acceptance of a collapse has no q / J test and no gate on short edges (`CNativeMetric3D.cpp:571-574`,
     `CNativeMetric3D.hpp:17`). So the core, with the same thresholds 1.8 and 0.9, will remove many more points than the
     engine does today. The ON mesh will have fewer cells than the OFF mesh. This is intended. Report the numbers (12.4).
   - Each collapse deletes one nodal tensor. The target of the engine after the core is the P1 field on the mesh that the
     core leaves, and the final gates are measured against that field. The engine has the same property today (its own
     collapses); the core does it more often. It is a known property of this item, not an error.
   - FP0 part B measured the stand-alone `Refine`. Production calls `AdaptMetric`, which starts with a collapse pass
     (`EngineAdapt.cpp:45`). So the cost of the engine on a good mesh is not known yet. The ON runs of part C measure it.
   - The costs of FP0 A6 to A8 are costs of prototypes that sort all edges again in each pass. They are not a forecast
     for the core.

## 2. Part C first in words: where the core runs (the details are in section 10)

### 2.1 The option
New boolean config option `ADAP_NATIVE_LOCAL_CORE`, default `NO`, getter `GetAdap_Native_Local_Core()`. Copy the four places
of `ADAP_NATIVE_REPARTITION` (`CConfig.hpp:1321`, `:10492`, `CConfig.cpp:3309`, `:6446-6447`). Do not add an entry to
`config_template.cfg`.

### 2.2 The place
`AdaptWorker`, NRR:668-713, between the construction of `trial` (:691) and the first `SnapshotSource` (:692). At this place
the working cells are divided among the worker ranks and each row has its nodal tensors (import sets them,
`CNativeImport3D.cpp:1165`). The existing `SnapshotSource` then copies what the core leaves. No second snapshot is necessary.
The place is before the deadline statement (:695), so the time of the core is inside the time budget of the event.

### 2.3 When it runs (data only, no case label)
The core runs when ALL of these are true:
1. `config.GetAdap_Native_Local_Core()` (a local read; evaluate it FIRST);
2. `!input.composition && !construction && kernel.wall_heights.empty() && kernel.region_walls.empty()` (local, the same on
   all ranks): no wall height is requested and no composition is present. This is the default form of the steps H1, H2,
   H3, H7, H8 of the design;
3. each working row on each worker rank has `sensor_present` (one `workers.maximum` of the local "a row has no tensor").
   Evaluate 3 only when 1 and 2 are true.
With condition 1 false, NO statement of the core runs: no collective call, no timer scope, no change of
`kernel.caller_bytes`, `stats.maximum_native_bytes` or `control.memory_ceiling`. (Each collective call adds a
`communication` timer call; G2 and GQ compare these.) With 1 true and 2 or 3 false: print one row `NATIVE3D_LOCAL_CORE
rank= applicable=0` under `control.timings` and continue with the engine as today.
`RunStages` can call `AdaptWorker` a second time (normalization, NRR:565-590). The core then runs again by the same rule.

## 3. Files and build

- Public header `Common/include/adaptation/CNativeLocalCore3D.hpp`. Sources in a new folder
  `Common/src/adaptation/native_local/` (several short files; proposal: mesh arrays, measures, changes, pass, border shift).
- Own namespace (proposal `SU2NativeLocal3D`). NOT inside the unity file `CNativeEngine3D.cpp` and not in an anonymous
  namespace, so that unit tests call each part directly.
- Add the sources to the strict-arithmetic library `accurate_sum_lib` (`Common/src/meson.build:46-57`,
  `-ffp-contract=off`), not to `Common/src/adaptation/meson.build`. Reason: float decisions must not depend on the build.
- Types to use: `Owned`, `OwnedCell`, `FacetBinding` (`CNativeDistributed3D.hpp:15-33`), `Cell`, `Node`, `Tensor`, `Id`
  (`CNativeMesh3D.hpp`), `World` (`CNativeDistributed3D.hpp:13`; header-inline, usable from a new file).
- R9 (readable code, D 7.1a) applies to all of it: short functions, one purpose each, comments that say why.
- The core gets memory numbers (ceiling, caller bytes, bytes of one `Owned` row) as arguments. `OwnedStorageBytes`
  (`CNativeImport3D.hpp:58`) is called in NRR, not in the core (the core's library is linked before `common`).
- The float kernels of the FP0 test file (`CNativeFastPathStage0_tests.cpp:318-361`: `FloatOrientation`, `NormSquared`,
  `ClosedLength`, `FloatLength`, `FloatShape`), its point copy with the bitwise check (:148-173) and its neighbour build
  (:181-220) are a sound start. Copy them into the core (leave the FP0 test file unchanged). Do NOT carry over: fixed counts, the global sort in each pass,
  tie-breaks by local index, the threshold 0.5, the renumbering of points.

## 4. Data of one rank (part A)

Flat arrays, built from `Owned` at the start of a pass and written back at its end.

| Array | Content |
|---|---|
| point | id (`Id`), coordinates (3 double), tensor (6 double), flags (surface, border, removed), one incident live cell |
| cell | 4 point indices (u32), 4 neighbours (i32), id, version, flags (live, changed, stamp of the sweep) |
| bindings | a sparse table: only for a cell with a physical face, its 4 `FacetBinding` (index in the cell row, -1 = none) |
| free lists | removed point and cell slots are used again inside a pass |

- Neighbour of face k (the face opposite point k): `>= 0` a local cell; `-1` a physical face (`physical[k].present`);
  `-2` a face at a border between ranks. A face with no local neighbour and no binding is always a rank-border face
  (import and `Repartition` enforce a conforming manifold: `CNativeImport3D.cpp:1229-1236`, `CNativePartition3D.cpp:274-281`).
- Build: copy the points (one entry for each id; coordinates and tensor of an id must be bit-equal in all its cells, else
  fail), sort the face keys one time, pair them. Then EACH change keeps the table up to date (section 6.1). No global
  sort after the build.
- Point flags. `surface`: the point is on a face with neighbour `-1`. `border`: the point is on a face with neighbour `-2`.
  This local rule finds each point and each edge that has cells on another rank: the cells around a point (or an edge)
  are connected through faces that contain it, so a part of them on another rank shows as a `-2` face here.
- A border point can be on the physical surface through a face of ANOTHER rank only. So, with more than one worker rank,
  the meeting exchange of section 9 (step 2) runs before each pass and also returns "a rank has this point on a `-1`
  face"; such a border point gets the flag `surface` too. With one rank there is no border and no exchange.
- "Free point" = not `surface` and not `border`. All cells around a free point are on this rank.
- "Wall-near point" = a free point that has a `surface` point as a neighbour (they share an edge). Compute this mark at
  the start of each collapse sweep.
- Memory: see 10.3.

## 5. The measures (double arithmetic, the numbers of the gates)

| Number | Rule | Source of the rule |
|---|---|---|
| Edge length | `first = norm(a,b,M_a)`, `last = norm(a,b,M_b)`, `g = max`, `r = min / g`, `L = g * (2/3) * (1 + r + r*r) / (1 + r)` | `CNativeField3D.cpp:468-472`, one piece (current nodal mode, `:267-273`) |
| Cell tensor | the arithmetic mean of the 4 nodal tensors | the gate asks the P1 value at the centroid (`CNativeMetric3D.cpp:243`) |
| q, J of a cell | as `Measure` (`CNativePredicates3D.cpp:922-985`) with the cell tensor | FP0 A3: difference 1e-15 |
| Validity of a cell | the production `Orientation` (`CNativeMesh3D.hpp:54`) is positive. It is exact and already filtered (34 ns). Do not write a guard band | FP0 A2 |
| Margin of a cell | `m = min(q / MinimumMeanRatio, J / MinimumScaledJacobian)`. The cell passes the quality gates when `m >= 1` | `CNativeMesh3D.hpp:97-98` |

Thresholds (constants of the core, with the names of the production constants in their definitions):
- `S = MaximumMetricEdge * (1 - 1e-12)` (= 1.8): an edge with `L > S` is "long".
- `c = CoarseningMetricEdge` (= 0.9): an edge with `L < c` is "short". Note `c = S / 2`: the two halves of a long edge
  are not short at once.
- A cell with `m < 1 + 1e-12` is "bad".
The margins 1e-12 come from FP0 A3 / A4 (the float numbers are the gate numbers to 1e-15). No other margin.

## 6. The changes (part A)

Rule of the user (validity first): in the length work (collapse, split) a change is refused only for validity or size. No
q / J number is read there. The quality work (6.4 to 6.6) reads q and J and never makes a cell invalid or an edge long.

### 6.1 One operator for all topology changes: `ReplaceCells`
Input: a set of old live cells (the cavity) and a list of new cells (4 point indices each, in the order that the rules
6.2 to 6.5 give). `ReplaceCells` NEVER changes the order of the points of a new cell.
Orientation of a face: face k of the cell `(v0,v1,v2,v3)` is the triple `(v1,v2,v3)`, `(v0,v3,v2)`, `(v0,v1,v3)`,
`(v0,v2,v1)` for k = 0..3. Two triples with the same three points have the same orientation when one is an even
permutation of the other (the convention of the face check of `Repartition`, `CNativePartition3D.cpp:83-87`, `:249-281`).
1. Collect the OUTER faces of the cavity: each face of an old cell that is not shared by two old cells. Key = its three
   point indices, sorted, with the orientation that it has in its old cell. Each outer face keeps its neighbour value
   (`>= 0`, `-1`, `-2`) and its binding.
2. Each face of a new cell must match exactly one of: (a) an outer face that is not yet matched, with the SAME
   orientation as in its old cell: it takes its neighbour and binding, and the neighbour cell points back to the new
   cell; (b) one face of another new cell, with the OPPOSITE orientation: the two are neighbours, no binding.
3. Each outer face must be matched exactly one time. Each new cell must be valid (section 5) in the order given.
4. If 2 or 3 fails: change nothing and return "refused". This check is the topology proof of each change. Reason (write
   it as the comment): with 2 and 3 the oriented boundary of the new cells is the oriented boundary of the cavity, and
   all new cells are positive, so the new cells fill the cavity exactly one time. This excludes an overlap with a cell
   outside, a double cell, and a new edge or face that exists outside already. Count the refusals by reason.
5. On success: remove the old cells, add the new cells (free list), set the flag `changed` and the stamp of the sweep on
   the new cells, correct the incident cell of each touched point.
Consequences (state them in the comments): a surface face and a rank-border face are always outer faces, so they are
never changed; a binding moves with its face (example: when a collapse removes the cell that holds a surface face, the
cell that takes this face takes its binding).

### 6.2 Collapse of a short edge (remove point `a`, keep point `b`)
- `a` can be removed when it is free and not wall-near. Reason for "not wall-near": the surface is not yet at its target
  in this stage (rule "surface first" of the user). The cells that touch the surface are not changed by a collapse.
  Stage 2 removes this rule. `b` can be any neighbour of `a`.
- Cavity = all cells around `a`. New cells = the cells around `a` that do not contain `b`, with `a` replaced by `b` in
  the same position (this keeps the orientation).
- New edge = an edge from `b` to a neighbour `x` of `a` that did not exist before the change.
- Refused when: `ReplaceCells` refuses; or a new edge has `L > S`.
- If both ends can be removed: evaluate both directions. Take the one whose longest new edge is shorter (no new edge:
  the value 0). Equal: remove the point with the larger id. If one direction is refused, take the other.

### 6.3 Split of a long edge at its midpoint
- Allowed when the ring of cells around the edge is closed on this rank: the walk around the edge meets no `-1` and no
  `-2` face. (So a surface edge is never split. An interior edge between two surface points can be split.)
- New point: coordinates `0.5 * (x_a + x_b)` for each component, tensor `0.5 * (M_a + M_b)` for each component (the
  engine's value for a split point is the same P1 value to rounding, `CNativeMetricTransport3D.hpp:68-129`). New id: section 8.
- Cavity = the ring. New cells = each ring cell two times: one time with `a` replaced by the new point, one time with
  `b` replaced, each in the same position.
- Refused only when `ReplaceCells` refuses (a new cell is not valid because of rounding) or the cell limit of 10.3 is
  reached. No length test.

### 6.4 Remove an edge (quality work; ring of 3 to 7 cells, closed on this rank)
- Names: `a` = the end with the smaller id, `b` = the other end. Ring points `r_0 .. r_(n-1)`: `r_0` = the ring point
  with the smallest id; the direction of the walk is the one for which the ring cell `(a, b, r_0, r_1)` is positive.
- Each triangulation of the polygon `r_0 .. r_(n-1)` gives the new cells. The triangle `(i, k, j)`, `i < k < j`, gives
  the two cells `(r_i, r_k, r_j, b)` and `(r_k, r_i, r_j, a)`, in this order of points.
- Best triangulation by the standard recursion over the polygon: `V(i,j) = max over k (i < k < j) of
  min(V(i,k), V(k,j), m of the two cells of the triangle (i,k,j))`, `V(i,i+1) = +infinity`. A cell that is not valid,
  or a triangle side that is a diagonal of the polygon (the only new edges) with `L > S`, counts as `-infinity`.
  Equal values: the smaller `k`. Result `V(0,n-1)`.
- Accepted when `V(0,n-1)` is larger than the smallest `m` of the ring cells, the cell limit of 10.3 holds, and
  `ReplaceCells` accepts.

### 6.5 Swap a face (quality work; 2 cells to 3 cells)
- The face between two local cells (neighbour `>= 0`). Names: `x < y < z` = its points in the order of their ids;
  `p` = the opposite point for which `Orientation(x, y, z, p)` is positive; `q` = the other opposite point.
- New cells, in this order of points: `(q,p,x,y)`, `(q,p,y,z)`, `(q,p,z,x)`. Refused when the new edge `pq` has `L > S`.
- Accepted when the smallest `m` of the 3 new cells is larger than the smallest `m` of the 2 old cells, the cell limit
  of 10.3 holds, and `ReplaceCells` accepts. (When the two old cells together are not convex, a new cell is not
  positive and `ReplaceCells` refuses.)

### 6.6 Move a free point (quality work)
- Rule of the engine, with its arithmetic (`EngineTrial.cpp:1354-1366`): the neighbour points in the order of their
  ids; the sums in `long double`; mean = sum / count; new place `double(p + step * (mean - p))` for each component;
  steps 1, 0.5, 0.25; take the first step that is accepted.
- Tensor at the new place: the P1 value in the OLD cells around the point, in the old cell with the largest minimum
  weight, weights clipped at 0 and scaled to the sum 1: the rule of `EvaluateCavitySample`
  (`CNativeMetricTransport3D.hpp:68-97`). Call that function if its interface fits; else write a twin and compare it in a fixture.
- Accepted when: each cell around the point is valid at the new place; no edge at the point has `L > S`; the smallest
  `m` around the point is larger than before.
- The point gets a NEW id (as `MoveVertex` of the engine does; an id never has two coordinates). The cells around it are
  "changed" and get the stamp of the sweep. The topology does not change, so `ReplaceCells` is not used.

## 7. One pass on one rank (part A)

All orders below are fixed. No decision reads a clock (the clock is read only under `control.timings`, for the print).
Each tie-break uses the global ids of POINTS (never a cell id, a local index or an address).

Length work:
1. Collapse sweep. List each short edge that has an end that can be removed. Sort by (L ascending, smaller id, larger id).
   Go through the list. Skip an entry without a count when one of its ends was removed in this sweep. Skip an entry
   ("deferred") when a cell around an end that can be removed has the stamp of this sweep. Else try 6.2.
2. Repeat step 1 until a sweep accepts no change (limit 50 sweeps).
3. Split sweep. List each long edge with a closed ring. Sort by (L descending, smaller id, larger id). The same stamp
   rule. Try 6.3.
4. Repeat step 3 until a sweep accepts no change (limit 50 sweeps).
5. Repeat steps 1 to 4 until one round accepts no change (limit 8 rounds).

Quality work:
6. Quality sweep. List each bad cell. Sort by (m ascending, the 4 point ids of the cell in ascending order).
   For each entry whose cell is still live and without the stamp of this sweep:
   a. Evaluate each removal of one of its 6 edges (6.4) and each swap of one of its 4 faces (6.5). Fixed order: edges by
      their sorted point ids, then faces by their sorted point ids. "Result" of an option = the smallest `m` of its new
      cells. Apply the accepted option with the largest result; equal: the first.
   b. If no option of (a) is accepted: try 6.6 for its free points, in the order of their ids. Stop at the first accepted.
7. Repeat step 6 until a sweep accepts no change (limit 20 sweeps).

The limits are safety limits. A limit that is reached is printed (`limit=`), it is not a failure. A sweep may list its
candidates by a full scan of the live cells; a faster method is allowed only with a fixture that shows the same result.

Self-check at the end of the pass (always, it is cheap): each live cell is valid (`Orientation`); the neighbour table is
symmetric; each `-1` face has a binding; no surface point and no border point changed its coordinates or tensor. A failed
self-check is a failure of the event (10.5).

## 8. New ids and writing back (part A)

- Before the first pass: `base_point` = the maximum point id and `base_cell` = the maximum cell id over the working cells
  AND the donors `*mesh.worker_source` of all worker ranks. Use a 64-bit collective maximum on `workers`
  (`workers.reduce<uint64_t>(value, CPassiveComm::Op::MAX)`, or `CPassiveComm::Allreduce` as at
  `CNativePartition3D.cpp:106`); `World::maximum` is for `int` only. The engine's own rule (`EngineSetup.cpp:202-222`)
  is weaker; this one is a superset and is safe.
- Worker rank `r` of `P` takes the point ids `base_point + 1 + r + k * P`, `k = 0, 1, 2, ...` in the order of creation
  (split points and moved points use the same counter), and the cell ids `base_cell + 1 + r + k * P` in the order of the writing back (ascending cell slot). The counters `k`
  continue in the second pass. An id is never used again. Guard against overflow of `Id`.
- Writing back, IN PLACE in the `Owned` map (do not build a second map): erase the row of each removed or changed cell;
  insert one row for each new or changed cell: 4 nodes (id, coordinates), 4 tensors, `sensor_present = true`, 4 bindings
  from the sparse table, new id, `version = 0`, positive order. A cell that the core did not touch keeps its row bit for
  bit (id, version, tensors, bindings).
- "Changed" = a point of the cell was replaced, or a point of the cell got new coordinates, a new tensor or a new id.
- The ids need not be dense. They decide the numbers of the points in the output files (`CReaderSlices.cpp:394, 486-540`),
  so the fixed order above is what makes two runs equal.

## 9. Borders between ranks: two passes and a shift (part B)

With one worker rank: one pass, no exchange, no shift. With more:
meeting 1, pass 1, shift "lower", meeting 2, pass 2, shift "higher".

Why a shift and not `Repartition`: `ParMETIS_V3_PartKway` (`CNativePartition3D.cpp:334`) makes a new partition from
nothing. It moved 46 % and 38 % of the cells in the reference events, and it does not guarantee that a frozen border
point becomes free.

`MeetBorderPoints(workers, working)` (the meeting exchange), as in `VertexStarSizes` (`CNativePartition3D.cpp:106-137`):
1. Local: the border points (section 4) from a neighbour table of the `Owned` rows.
2. Each rank sends (point id, own rank, "this rank has the point on a `-1` face") for each of its border points to the
   rank `id % P`. That rank finds, for each id, the smallest sending rank, the largest sending rank and the OR of the
   surface marks, and sends (id, smallest, largest, surface) back to each sender. Use `World::exchange` on `workers`
   with `ceiling = kernel.memory_ceiling` and the baseline `kernel.caller_bytes + OwnedStorageBytes(working)` plus the
   live buffers, as that function does. A rank with zero cells takes part in each collective call.
The result of a meeting gives the `surface` mark of the border points for the pass after it (section 4) and the ranks
for the shift after that pass. A pass does not change a border face, so the result is still right at the shift.

`ShiftBorders(workers, working, meeting, direction)`:
3. Destination of a local cell: direction "lower": the minimum, over its border points, of the smallest rank of the
   point; direction "higher": the maximum of the largest rank. A cell without a border point stays.
4. Move the rows with `World::exchange<OwnedCell>` and build the map again, with the admission of the new map (as the
   migration inside `Repartition`, `CNativePartition3D.cpp:353-390`). A row is moved bit for bit. Do NOT touch
   `stats.partition` (a later normalization reads its `destinations`, NRR:1079-1082). Do not move donors: the worker
   does not read them (section 1, fact 1).

Properties (prove them in the fixtures):
- The union of the rows of all ranks is the same before and after a shift. No row is lost or made two times.
- A border point of pass 1 that is on exactly two ranks `i < j` and is not on the surface is free in pass 2, when no
  cell around it has a border point whose smallest rank is below `i`. (Near a place where three or more ranks meet this
  condition can fail; such a point can stay frozen in both passes.)
- `frozen_in_both` = the count of ids that the rank `id % P` receives in both meetings. It is printed at the second
  shift. The engine does what is left there.
The second shift ("higher") brings the cells of each rank back near their count before the core. This is a description,
not a gate. A rank can have zero cells after a shift on a small case; the engine accepts that
(`EngineSetup.cpp:215-218` checks the global count only).

## 10. Part C: the call and its rules

### 10.1 The call
A new member function of the run class beside `SnapshotSource` (proposal `RunLocalCore(RemeshInput&, WorkerMesh&,
WorkerTrial&)`), called between NRR:691 and :692 under the conditions of 2.3. Line :692 becomes
`if (valid) valid = SnapshotSource(...)`. The function: id bases (8), then for each pass: meeting (9; not with one
rank), admission (10.3), build the arrays, run the pass (7), write back (8), free the arrays, elect a failure (10.5),
shift (9; not with one rank). The count of passes is ONE named constant in the core header (value 2; with one worker
rank the core makes one pass). All collective calls use
`trial.workers`. NEVER use `parent`, `Admit` or `Vote` of the run class there: the idle ranks wait in `JoinWorkers`.

### 10.2 Timers
At that place only the root timer session `remesh` is open, in the phase `preparation` (`CNativeRemesher3D.cpp:213`). So
`RunLocalCore` opens `SU2NativeTiming3D::PhaseScope(Phase::Construction)` and inside it
`SU2NativeTiming3D::Scope(Operation::Topology)` for all its work. Both are existing values: no new operation, no new
phase, no new field; `fixedstorage=4120` stays (R10). The collective calls of the core then appear as
`construction / communication`. The 95 % coverage gate does not change. (The time of the core is then in the phase
`construction` of the timer rows and in `other=` of the `NATIVE3D` row. Do not add a name to that row.)

### 10.3 Memory
- Admission before the arrays of a pass are made, on each rank: `kernel.caller_bytes + OwnedStorageBytes(working) +
  (bytes of the core arrays for the present counts, with `transfer_memory::GrowthBound`)` must not be above
  `kernel.memory_ceiling`. Pattern: `SnapshotSource` (NRR:722-754): a local failure, then `workers.elect`. Raise
  `stats.maximum_native_bytes` with this value. Do not change `kernel.caller_bytes`.
- Cell limit of a pass (a limit on the count of live cells; it is NOT an allocation: do not reserve it):
  `limit = (kernel.memory_ceiling - kernel.caller_bytes) / (bytes of one `Owned` row + bytes of the core arrays for one
  cell)`, where the second term is one named constant bound of the core. A change that would raise the count of live
  cells above the limit is refused and counted (`refused_memory`): split, face swap, edge removal. Deterministic.
- The arrays grow as the work needs (normal vector growth). The 19073 MiB rule of the job is the measured peak
  (`JoinWorkers`); it does not change.

### 10.4 Prints (under `control.timings` only, R11)
One row for each rank and pass:
`NATIVE3D_LOCAL_CORE rank= stage= pass= applicable=1 cells_in= cells_out= points_in= points_out= surface_points=
border_points= wall_near_points= collapses= splits= edge_removals= face_swaps= moves= deferred= refused_valid=
refused_size= refused_topology= refused_memory= long_left_surface= long_left_border= short_out= edges_out= bad_in= bad_out=
bad_out_free= sweeps_collapse= sweeps_split= sweeps_quality= rounds= limit= seconds= bytes=`
(`stage` = `stats.normalization_stages - 1`; `bad_out_free` = bad cells with 4 free points; `long_left_*` = long edges
that the pass could not split, by reason; `short_out` = short edges at the end of the pass, `edges_out` = all edges.)
One row for each rank and shift: `NATIVE3D_LOCAL_SHIFT rank= direction= cells_before= cells_after= sent= received=
border_points= frozen_in_both= seconds=` (`frozen_in_both` at the second shift only, from the rank `id % P`). No decision
reads these values.

### 10.5 Failure
A failure inside the core (an exception, a failed self-check, a failed admission) is elected on `workers` and ends the
event with its message, as the other failures of `AdaptWorker` do. There is no silent return to "engine only". A change
that is refused is not a failure. Catch `std::exception` locally and elect it. `CElectedFailure` is not a
`std::exception` (`CDistributedSearch.hpp:216`): let it pass up to the handler of `AdaptWorker`; do not elect it again.

### 10.6 Not changed
The engine, its options and its gates; `SnapshotSource`; `stats.partition`; the donors; the boundary parameter state (it
has no record for an interior point); transfer, output, restart; the default of each existing option.

## 11. Fixtures (new test files, new tags that G1 does not select; R3)

Part A (one rank; a box of tetrahedra as `Grid(n)` of `CNativePartition3D_tests.cpp:28-55`, with a tensor field that is
not uniform, so that the box has long edges, short edges and bad cells):
1. Each change of section 6 on a small hand mesh: valid result; the neighbour table is equal to a table built again from
   nothing; bindings are on the same faces as before.
2. `ReplaceCells` refuses a wrong replacement (a face matched two times, a face not matched, a cell not valid, a face
   matched with the wrong orientation) and leaves the arrays bit-equal. One case must be the face swap of two cells that
   together are not convex (the line `pq` does not cross the face): refused.
3. The measures against production on all cells and edges of the box: edge length against
   `FrozenField(..., current_nodal = true).TargetEdgeLength`; q and J against `Measure`; largest difference below 1e-12.
4. One pass. After the length work (step 5 of section 7): no long edge with a closed ring is left and no short edge is
   left whose collapse section 6.2 accepts (both: unless `refused_valid`, `refused_memory` or `limit` is not 0; then
   show that each such edge has that reason). After the quality work: the smallest `m` over all live cells is not below
   its value before the quality work; no edge is long that was not long before it. At the end: ids unique; one tensor
   for each point; each surface point, surface face, border point: bit-equal; the self-check passes.
5. Round trip: `Owned` to arrays and back with no change gives the same map bit for bit.
6. Same result: two runs give identical maps; a run with the cells given other ids (so another build order) and the
   same point ids gives the same set of cells and the same new point ids (compare the cells by their sorted point ids).
7. The strict `MeasureMetricCells` on the result: its counts of q-bad, J-bad and long-edge cells are equal to counts
   that the test makes with the measures of the core and the gate thresholds without a margin (q < 0.20, J < 0.05, a
   cell with an edge `L > 1.8`). Report the cells with `m` in [1, 1 + 1e-12) and the edges with `L` in (S, 1.8] apart.
8. Wall-near rule: run the collapse sweeps only (steps 1 and 2 of section 7). No cell that touches a surface point was
   removed or changed.

Part B (2 and 4 ranks; divide the box in slabs by the x coordinate of the cell centre, so that each rank has one
compact region; the round-robin `Layout` of the test file puts almost each point on all ranks and is not useful here):
9. After each shift: the union of the rows over the ranks is bit-equal to before; each `-2` face has exactly one partner
   on another rank (`Repartition`'s face check, or the output check of `CReaderSlices`).
10. Each border point of pass 1 that meets the condition of the second property of section 9 is free in pass 2 (the
    test evaluates the condition itself). The count `frozen_in_both` is equal to an independent count in the test.
    A border point that is on the surface on another rank only has the flag `surface` on each rank.
11. The full sequence (pass, shift, pass, shift) on 2 and 4 ranks: the union is a valid conforming mesh; fixture 4 holds
    for each point that was free in a pass; two runs give identical rows.
12. The engine accepts the result: `AdaptMetric` (or `Refine`) on the output of fixture 11 ends valid.

Part C:
13. A config-driven remesher test (style of `CNativeImport3D_tests.cpp:1177`) with `ADAP_NATIVE_LOCAL_CORE= YES` at 1, 2
    and 4 ranks on a small case: the event is accepted; the row `NATIVE3D_LOCAL_CORE applicable=1` is printed with changes
    above zero; two runs give identical output. The same case with a wall height requested (as
    `CNativeRemesher3D_tests.cpp:914, 945`) prints `applicable=0`.
14. The default of the option is NO (as `CNativeSupport2D_tests.cpp:18`).

## 12. Gates

### 12.1 Part A and part B (each)
1. Build without a new warning. `git diff` to the base: only the new files of section 3, the list in
   `Common/src/meson.build`, test files and `UnitTests/meson.build`, your evidence folder.
2. The fixtures of the part pass.
3. G1: the tables are identical to the reference (new tests have new tags).
4. No G2, GQ, G3, G5 for these parts: no production statement calls the new code. Commit and push each part.

### 12.2 Part C, option OFF (the default): an item with identical results
1. Build without a new warning. `git diff` of part C to part B: only the four `CConfig` places, the call and the new
   function in NRR, a change inside the core files, test files, your evidence folder.
2. Fixtures 13 and 14 pass.
3. G1, G2, GQ, G3 (identity part) and the safeguards of section 2 of `AGENTS.md`.
4. ONE G5 run with the option OFF (the pinned config, not changed): all standing gates, six files identical to the
   reference in force (section 2a). Its times are the "OFF times of this night".
The ON runs start only after 1 to 4 passed.

### 12.3 Part C, option ON: an item that changes the mesh
Directly after the OFF run, with the same binary: TWO G5 runs with the option ON.
How: copy the pinned `run.cfg` of G5 into `FP1/`, add the one line `ADAP_NATIVE_LOCAL_CORE= YES`, record the SHA256 of
the copy, and bind your runner to it (test code, R3). The one file drives both events. No other change.
1. All standing gates of G5 pass in both runs: 5400 s for each event; 19073 MiB with the job peak equal to the sum of the
   process peaks; the physical validator; transfer; continued CFD; the restart checks of the N22 G5 receipt; 95 % timer
   coverage on each rank; `fixedstorage=4120`; join agreement; owned proof faces SUM equal to the surface size.
2. The six files are identical between the two ON runs. They are then the reference of the ON path
   (`R/night_20261115/FP1/on_reference.json`). The reference in force of `AGENTS.md` (OFF) does not change. The default
   of the option stays NO; only the user changes it.
3. Time rule: the native maximum of each event of each ON run is not above the OFF time of this night times 1.05. A miss
   of this rule alone is "OPEN for the user", not a failure and not a revert.
4. The checks of your runner that compare with the OFF reference (identity of the six files with the reference in
   force, the first-collapse work and count gate, the prefix gates of P7 / P8) are a REPORT for the ON runs, not a
   gate. They differ by design. The gates of the ON runs are exactly 1 to 3.

### 12.4 Report rows for part C (OFF beside ON, each event)
Cells and points of the final mesh; `local_edges` and `short_local_edges` of the last `NATIVE3D_LENGTH` row, summed over
the ranks; the rows of 10.4 summed over the ranks; the engine's accepted collapses and splits and its count of rounds;
the seconds of each phase (preparation, construction, quality, final proof), with the `seconds=` of the core and of
the shifts shown apart from the engine's part of `construction`; `Native3D frozen stages: count=` and the complexity
value (did a normalization stage run?); the smallest timer coverage; the job peak; cells of each rank at the entry of
the engine.

### 12.5 If a gate fails (R5, with these additions)
- A gate of part A or B fails: R5 for that part.
- An OFF gate of part C fails: R5; a revert is a revert of part C only. Parts A and B stay.
- ON gate 1 or 2 fails: ONE diagnosis pass (R5), then EXACTLY ONE repeat of the two ON runs:
  either after the correction of a wiring or transcription error, or, when the diagnosis names the border shift as the
  cause, with the count of passes set to 1 (the constant of 10.1; then meeting 1 and pass 1 run, and no shift runs). The OFF run is not
  repeated (the OFF path does not change); the time rule uses the OFF times of this night.
  Any other cause, or a failure of the repeat: do NOT revert parts A to C (OFF is proven identical and is the default).
  Mark "FP1 ON: FAILED" with the evidence and the first failing number, and stop the item.

## 13. Report and close

`R/night_20261115/FP1/reports/FP1_<build>.md`: the source check list (R4, "reading used"); for each part its gates and
fixtures; the tables of 12.4; each R3 correction; each question (R13). Update the progress file, `HANDOFF_Codex.md`,
`HowToProceed.md`, `Handbook/ADAPTATION_TIMINGS.md` (a row for the core and the shift; the engine phases OFF beside ON).
Commit, push, verify that no owned job runs, stop and wait for the user.

## 14. Not in this item

Any change of the surface or of a surface point. Wall layers (H5, H6). A change of the engine, of a gate, of a threshold
or of a default. A new timer operation or phase. `Repartition` after the core. Removal of the wall-near rule. A faster
candidate search without its fixture. The fixed parts of stage 3 (mobility policy, census, proof, output).
