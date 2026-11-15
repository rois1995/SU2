# Design v1: one adaptation engine for no-BL, BL, 2D, 3D and MPI

Author: the reviewer (read-only session), 2026-11-14. Status: PROPOSAL for the user.
The main agent does not change this design. It checks it against the source, measures the
numbers of section 9, and implements after the user accepts.

Evidence annex: `M6_STAGE3_WALL_FIRST_DESIGN_BRIEF.md`, sections 3.1 to 3.9 (cited as "A3.x").
Two new read-only audits of 2026-11-14 are folded in (MPI layer; wall split and target).
Status letters: M = measured, C = read in the code, I = inferred, A = assumed (to be measured).

Short names: Eng = `Common/src/adaptation/CNativeEngine3D.cpp`, Bnd = `Common/src/adaptation/CNativeBoundary3D.cpp`,
BP = `Common/src/adaptation/CNativeBoundaryParameters3D.cpp`, AR = `Common/src/adaptation/CNativeAffineReference.cpp`,
BE = `Common/src/adaptation/CNativeGeometryBackend3D.cpp`, Dist = `Common/src/adaptation/CNativeDistributed3D.cpp`,
DH = `Common/include/adaptation/CNativeDistributed3D.hpp`, Part = `Common/src/adaptation/CNativePartition3D.cpp`,
MT = `Common/include/adaptation/CNativeMetricTransport3D.hpp`, Pipe = `SU2_CFD/include/adaptation/CNativeBLPipeline3D.hpp`,
BLM = `SU2_CFD/src/adaptation/CBoundaryLayerMetric.cpp`.

## 1. What does not change

- All gates: geometry 1e-4, q, J, size, first height (10%), transfer, restart. Budgets: 5400 s
  total, 12 GiB. Requested heights and progression. CAD stays optional.
- Validity first: each intermediate mesh has exact positive nondegenerate cells, correct
  topology, wall points on the surface, body separation, genuine features.
- The gathered serial MMG path stays the default.
- The mathematics of the first row (direction fields, kernel bounds, apex LP, column registry).
- The exact predicates, the interval geometry proof, the cavity operators. This design changes
  WHEN and HOW OFTEN they run, not what they prove.

## 2. The model in one page

**Requirements are data.** One record describes a case:

| Requirement | Content | Empty means |
|---|---|---|
| Size | metric field carried on the cells | not permitted |
| Geometry | authority, tolerance tau | flat walls only |
| Features | mobility policy (corners, protected edges, sheets) | no features |
| Wall rows | for each wall: first height, growth, thickness | no BL: row stage does no work |
| Budgets | time, work, memory | not permitted |

**One stage sequence for all cases.** No stage is chosen from a case label, a solver label or
`ADAP_BL_METHOD`.

| Stage | Work | Runs when |
|---|---|---|
| S0 Target | compose size, geometry law (section 3) and row demand in ONE place | always |
| S1 Wall | bring the wall surface to the target: need rule, insert, certify (section 4) | walls exist |
| S2 Rows | first row and growth on the final wall | a wall-row requirement exists |
| S3 Volume | collapse, split, swap to the target | always |
| S4 Quality | shape improvement | always |
| S5 Final | all final gates, final proof, transfer | always |

**One rule for each edit** (construction stages S1 to S3): an edit is applied when (a) a
NEED rule that uses only data at the points of the edit says it is needed, and (b) the result
is VALID. Nothing else can veto it. Quality and progress are measured after, not for each edit.

**One parallel method** (section 5): inside a pass each rank works alone on the cavities that
are complete on that rank. The ranks exchange data between passes.

**Three layers** (section 7): a dimension-free core, one kernel for each dimension, the
requirements.

## 3. The geometry law (stage S0) - derived, no tuned factor

Problem (C): the curvature size cap exists only in the BL wall metric (BLM:2393-2407), it is
applied to each principal direction alone (BLM:192-209), and it cannot be reached with no BL
(`MeasureSurfacePlanning` throws with no walls, `CNativeMetric3D.cpp:349-351`). Debt:
`R/joint_curvature492_debt.json`.

Data that exists (C): `MappedFacetReference::EvaluateSurface(side, binding)` (AR:1937-1990)
returns the point, the normal n, the principal curvatures k1, k2 and their directions in
closed form from the cubic control net. Cost O(1). No CAD. There is NO certified curvature
bound over a facet; so the law below is an estimate that steers. The proof stays in S5.

Derivation. Near a wall point the surface is its osculating paraboloid with second
fundamental form II. Take a flat triangle with corners on the surface, edge vectors e_ij and
barycentric coordinates lambda. The normal distance between triangle and surface is

  d(lambda) = 1/2 * sum over i<j of lambda_i lambda_j (e_ij' II e_ij).

Let |II| be II with absolute principal curvatures, and q(e) = e' |II| e >= 0. Then
d <= 1/2 * max q * sum lambda_i lambda_j, and the sum is at most 1/3 (centroid). So:

  **if q(e) <= 6 tau for the three edges of a wall face, then d <= tau on the face.**

On one edge alone the sagitta is q/8. So the face rule is the edge rule with constant 6
instead of 8, and it holds in EACH tangent direction because |II| is a tensor.

Edge estimate, from the end points a, b only (L = chord length, e^ = unit chord):

  s(e) = max( q_a(e)/8, q_b(e)/8, (L/4) |n_a . e^|, (L/4) |n_b . e^| )

The first two terms are the curvature form. The last two use the angle between the chord and
the tangent plane; they are equal to the curvature form for constant curvature, and they see
curvature that lies BETWEEN the end points (an edge across the leading edge), which the
point curvature cannot see.

Need rule for a wall edge: **geometry need = s(e) > (3/4) tau** (this is q > 6 tau).
The metric form for S0 is M_geo = |II| / (6 tau); it is composed with the size metric and the
row demand, so the volume stages see it. The geometry need is a hard rule (limit 1), not the
1.8 size gate.

The law lives in the shared target, not in the BL code. Every path gets it.

## 4. Stage S1: the wall, without trial and error

Problem (M, C): run 492 made 305 splits in 389 s. 72% of the rounds were deferrals. Each
trial paid a certified proof as a progress veto (175 s), a guidance evaluation (145 s) and BL
checks before a BL exists. See A3.3.

### 4.1 Need rule (replaces the search)

A wall edge is split when it has a geometry need (section 3) or a size need (its length in
the composed target is above the existing size limit). A wall edge is collapsed only when
each new wall edge has NO need by the same rule. So a split is never undone by a collapse.

The rule uses only the two end points and their stored surface data. So:
- nothing is found by trial;
- both sides of a rank interface take the same decision;
- child edges are tested at once, so a place is refined as many levels as it needs in one
  pass (replaces the entry-only frontier Eng:935, 280, 865 and the census-gated generation
  end Eng:3298, 3316).
Order: largest s(e) first (longest-edge bisection in the law metric). It ends because each
split divides q by 4.

The surface data of a point (n, k1, k2, directions, strip membership, guidance) is computed
ONE time when the point is created or first used, and kept with the point.

### 4.2 Insert (validity only)

Operator order (exists, C): `SplitEdge` = plain split of the complete shell with the new
point on the surface (Bnd:1562-1606); if it refuses, `InsertEdgeCavity` = star cone from the
new point (Bnd:1607-1675).

Checks that stay for each edit (validity, C): exact `Orientation > 0` and nondegenerate
cells (Bnd:1585, 298-304); `ValidatePrescribedInterface` (Bnd:309); `CheckSurface` /
`ContainsEntityFace` (Bnd:331, BP:304-313); feature binding and `RebuildFeatureTraces`; body
separation (`Clearance`, see 5.4); complete cavity.

Checks removed from S1 (they are target or progress checks, C): `PreservePassingFirstHeights`
(Eng:3979), `CheckFirstHeight` (Eng:3981), `geometric_change` with `GeometryGuidanceExcess`
on old and fresh (Eng:3992-4029), `MeasureSurfacePlanning` + `ValidateMetricChange` +
`metric_trial` (Eng:4424-4452), the growth loop of 12 growths x 3 frames (Eng:4994-5213),
the per-trial measurement of the old cavity (Eng:4401), `PredictGeometrySplit` ordering.

If both operators refuse an edge: keep the edge in a short "refused" list. Try it again
after its neighbours are refined (the shells are then smaller). What stays refused at the end
of S1 goes to the existing certified wall-split family as a repair. This list must be short;
its size is a reported number, not a gate.

Wall edits under existing rows (designed 2026-11-15; it was open). This is the normal case,
not a special one: each adaptation after the first one starts from a mesh with rows.
- Rule on data, not on a case label: a wall edit does not work on a wall vertex that
  carries a column. Before a wall edit, the operator `Unlift` removes the column at each
  wall vertex of the faces that the edit changes.
- `Unlift(v)` is the inverse of the row transaction at v (5A): it removes the apex of each
  wall face at v, then it collapses each column edge above v onto the point below, from the
  wall up. Each step is one general edge collapse with validity checks only. The state
  after each step is a partial row, which is a conforming mesh (X/BLFirstRow9:354-358, C).
  The exact step list must mirror `Transaction` (X/BLFirstRow9:691-765); the main agent
  reports it in the source check before step 8.6.
- After the wall edit, the changed wall faces are marked "rows needed". S2 builds their
  rows again. The neighbour columns are complete, so the step rule of 5A.2 item 4 lets the
  new columns grow to full height in one pass.
- Wall faces that S1 does not change keep their rows. A change of h0 only (the h0 ramp)
  uses the column rebuild of 5A.2 item 8, with no `Unlift`.
- If `Unlift` is refused at a vertex, the wall edit is refused and goes to the refused
  list below. Reported number (M14): wall vertices that S1 changes on a mesh with rows, and
  the share of refused `Unlift`.
- Retired with this: `RefitWallSplit`, `ReconstructWallSplit`, `ReconstructSplitPatch` and
  the growth loop of 12 growths (Eng:5003-5217), which re-fit apices under a wall split.

No new binding API is needed in v1. `BindNewPoint` re-projects a Cartesian point
(BP:1404-1407, C); the projected chord midpoint is used as today.

### 4.3 Certify (one time, same method for all faces)

- After S1 (provisional) and in S5 (final gate): each rank proves the wall faces it OWNS
  that have no certificate. One reduction gives the result. Today each rank proves the
  whole surface (A3.6: 69544 faces, 675 s on every rank).
- Correction 2026-11-15 (helper, C): the final gate of today is not a face-by-face
  `Classify`. It is the entity certificate: `GeometryCertificates` -> `PatchErrorBounds`
  (BP:412-445), one two-sided union query for each entity over all its faces, with
  `complete=true` in `finish` (Eng:1945), so nothing is reused. The gate must stay this
  same statement. The design runs the same function on the patch of owned faces that have
  no certificate. To verify before code (M15): the two-sided bound of an entity is the
  combination of the bounds of patches that cover it (each face has its own cover of the
  authority, BP coverage). If the source says no, the final proof stays for the whole
  entity, runs on ONE rank for each entity, and only the per-face part is split.
- The result is stored in the census row of the cell, with the cell version (the row already
  holds `geometry_scores` by version, Eng:300, 863). Rows travel with migration and go into
  the checkpoint. A face that did not change is not proved again.
- A face that fails is repaired in a loop (made exact 2026-11-15, user question):
  1. The proof gives the measured error E > tau of the face.
  2. One level of splits divides the gap by 4 (the gap goes with the square of the edge
     length). So the face needs n = ceil(log4(E / tau)) levels. All n levels are applied at
     once, with the insert of 4.2 (longest edge in the law metric first).
  3. Only the new faces are proved. A new face that fails goes to step 1 with its own E.
  The loop ends: E falls by a factor 4 for each level, and a collapse cannot undo a split
  (4.1). A split that both operators refuse goes to the refused list of 4.2. If the time
  budget ends with a failed face, the mesh is rejected; the gate does not change.
  Inside passes: the repairs of one proof run are the local edits of the next pass.
  Reported numbers (M13): share of faces that fail the first proof, and the largest E / tau.
- No sampling. No relaxed gate. The estimate of section 3 never accepts a mesh.

### 4.4 Where S1 runs

`PrepareSurface` is collective over all ranks already (C: the test fixture permits 6 ranks).
The one-rank limit comes from the serial first-row stack around it (Pipe:335). So S1 moves
out of `RunEvent` and runs on all ranks, also for a BL case. Until the first row is inside
the engine (8.6), the mesh is gathered on one rank after S1 for the serial row stack (I: this
needs a migration with an explicit destination, the same function as in 5.5).

## 5. The parallel method: passes

Problem (M, C): one edit for each rank in each global round; 140 to 255 elections and 69 to
98 collectives for each round; 66 to 90% of the CPU time of each rank inside the MPI library;
in each slow phase one rank computes and five wait (A3.6, A3.7). A rank has no halo and no
flag for shared vertices; it knows that a vertex is shared only from the directory, whose
record lives on rank hash(id) % ranks (Dist:123-131, DH:33).

### 5.1 Local rule (new, no message)

An edit is LOCAL on a rank when the complete cavity that its operator needs is made of cells
of this rank, and the outer skin of the cavity does not change:
- edge split, swap: the complete shell of the edge;
- collapse, relocation: the complete star of the moved vertex.
Test without the directory: each face of the cavity cells that contains the edge (or the
vertex) is matched by a second local cell or is a physical boundary face. The local index
exists (`Selection::vertex_cells`, Eng:318).

Faces, edges and vertices on a rank interface fail this test. They are frozen in the pass.
Two ranks can never change the same cell, face, edge or point. So no reservation is needed.

### 5.2 Inside a pass (each rank alone)

- The rank applies all needed LOCAL edits, in its own priority order, until none is left or
  its budget share ends. No lookup, fetch, reserve, election or vote.
- New ids: the token scheme exists and is rank-interleaved (Eng:3899-3904). Replace the
  global `round` term by a counter of the rank.
- Metric: carried on the cells and transported from the cavity (MT:130-153, 360-396). Local.
- Surface: the rank writes its surface and parameter changes into its private overlay and
  keeps the list of changes. Changes of two ranks touch different faces and points (5.1), so
  they commute.
- Budgets: the rank checks its own clock and memory for each edit (the pattern exists:
  Eng:1190-1193, 5438-5440, 2506-2508). Each rank has a share of the remaining work units.

### 5.3 Between passes (one collective sequence)

1. Reduce: needs left, work used, time, memory, failures.
2. Publish all surface and parameter changes of the pass in one exchange and apply them on
   all ranks. The surface state stays replicated in this design step (the agreement
   Dist:1423-1436 then runs one time for each pass, not for each edit).
3. Build the directory again from the cells (measured: 0.62 s for the M6 mesh on 6 ranks).
4. Collective lane for the edits of 5.4, in batches (the batch tools exist: `ReserveBatch` /
   `ChooseBatch` Dist:1127-1246, `ClearanceBatch` Eng:5747-5950).
5. Move the interface (5.5) and migrate (rows and certificates travel with the cells,
   Eng:2919-2937).
6. End when no rank has a need left, or the budget ends.

### 5.4 Body separation

`Clearance` is a contact test against the cells of all ranks (Dist:441-509). It is needed
only when new cells can leave the hull of the old cavity. Local proof: if the new wall point
is inside the old cavity (exact orientation tests against the faces of the cavity skin), the
new cells are inside the old cavity, and no contact is possible. This is the case on a convex
wall, because the chord is inside the body and the surface point is on the fluid side.
An edit that fails this local proof is not applied in the pass. It goes to the collective
lane of 5.3 item 4 with the existing batch clearance. On the M6 wing nearly all wall splits
should pass the local proof (A, measure M6 of section 9). A concave wall is correct but
slower. The exact form of the proof and its limits (wall collapses on a convex body fail
it always) are in 5.9 item 5.

### 5.5 Moving the interface

Only a balance-driven ParMETIS partition exists (Part:334); no function moves an interface on
purpose (C). New small function: each rank marks its cells within K layers of an interface.
For each pair of neighbour ranks the band of one side goes to the other side (deterministic
rule on the rank numbers, alternating with the pass number). The old interface is then K
layers inside one rank and is free in the next pass. K = layers of the largest cavity + 1.
The migration that exists moves the cells, given a destination for each cell (I: check that
it accepts an explicit destination list; ParMETIS gives it one today, Part:360-366).
Places where three or more ranks meet can stay frozen for two passes; what is left after
that goes to the collective lane.

Balance repartition and the interface move are one step (added 2026-11-15, user question).
Present rule (C, Eng:2790-2923): at a phase start, each cell gets a predicted demand, a
geometry cost and a quality debt; imbalance = ranks x max / sum; ParMETIS runs only if
imbalance > 1.25 and predicted gain > 2 x predicted cost (cost = estimate + census rebuild +
cache warming). Measured: it declined at imbalance 2.4 to 6 (A3.6).
1. Both steps give each cell a destination rank and use the same migration. Between two
   passes: first the balance rule; if it runs, ParMETIS gives the new partition. Then the
   band rule runs ON that result, only for pieces of the old interface that are still within
   K layers of an interface. One combined destination list, one migration for each pass.
   So the old interface is K layers inside a rank also when ParMETIS keeps a cut in place.
2. Inputs of the balance rule come from the reduce of 5.3 item 1: needs left and measured
   edits per work unit for each rank. This replaces the predicted-attempts proxy.
3. The migration is paid in each pass for the interface move. So the cost term of the
   balance rule is only the EXTRA cells that the balance moves. Keep the form
   "gain > 2 x cost"; do not tune the factor.
4. Pass length: each rank has a share of work units for the pass (deterministic, not
   seconds). A rank with no local need left waits only until the other ranks end their
   share. The first pass has a short share, so the balance rule gets measured rates early.
   The work units are the two counters that exist (C): the edit counter (`global_work`:
   +1 for each proposed edit, Eng:3596-3601; limit 64 x input cells, Eng:2087) and the
   geometry counter (`AtlasBudget::work`: +1 for each elementary step of a wall-geometry
   query, `Common/include/adaptation/CNativeAtlas3D.hpp:44-49`; the clock is read each 1024
   units). The pass share is counted in proposed edits. The load of a rank for the balance
   is: needs left x geometry units for each edit, measured on that rank in the last pass.
   Both are counts, so the partition and the mesh do not depend on the host load.
   The size of the share needs M4; rule: the pass overhead (directory + migration, about
   5 s, M) must be a small part of the pass. No number is fixed here.
5. The band move changes the balance by K layers of one interface; the direction alternates
   with the pass number, so the drift is bounded by one band.

### 5.6 What is reused, what changes, what is retired

Reused (C): private evaluation with zero collectives (Eng:5437-5491), local metric imports,
id tokens, aggregated publication (Eng:5815-5897), `PublishMetrics`, repartition and row
migration, local deadline pattern, batch reserve and batch clearance.
Changes: local cavity test; local apply without directory; publication split into "apply
now" and "merge at pass end"; budget shares; `measure` with no collective for each batch
(Eng:2655-2660); interface move; certification by owned faces.
Retired from the hot path: per-edit lookup / fetch / reserve at the hash homes, `coalesce`
(Eng:1368-1422), the vertex hold for the rest of a pass (Dist:221, 1229, 1296), per-round
global budget and epoch votes (Eng:3150-3482), receipts (Eng:1423-1476).
Not required and not promised: the same mesh for different rank counts (it is not required
today, C). For one rank count the result is deterministic.

Later step, not in v1: partition the current surface state with the mesh. For 10 million
cells the replicated surface is too large. It is not a limit for M6.

### 5.7 Expected speed

The interior lane reaches about 1000 to 1200 edits per second on 6 ranks today, in lockstep
(M). With no message for each edit, each rank should reach at least that alone (A, measure M4).
Target of the design: construction (S1 + S3) of the M6 no-BL case, about 115000 edits (M), in
120 s on 6 ranks = about 160 edits per second per rank. Edits per second must grow with the
rank count; the pass overhead (directory 0.6 s, migration about 4 s for 129k cells, M) is
paid a small number of times.

### 5.8 Threads (added 2026-11-15, user question; user decision 3 of section 10)

Fact (C): the native engine has no thread use today (no OpenMP in Eng, Part, AR; the
geometry cache has locks, AR:1117-1118). "Workers" are MPI ranks (Rem:202).
Proposal: write section 5 for a WORKER, not for a rank. A worker owns a zone of cells with
its own cell store, queues, counters and scratch memory. A rank is a worker. A thread is a
worker inside a rank. The local rule 5.1, the pass 5.2 and the interface move 5.5 are the
same for both. Between two workers of one process, cells move by memory, not by message.
Three rules for all new code from step 8.2 on, so that threads are a later step and not a rewrite:
1. An edit reads and writes only the data of its worker. Data shared by workers is read-only
   inside a pass (authority, frozen metric, published surface state).
2. No hidden shared state: counters, caches and scratch buffers belong to a worker. A shared
   cache with a lock in the hot path is not allowed.
3. New ids come from a counter of the worker; the token carries the worker number.
Gain expected from threads (I, not measured): the immutable authority and the replicated
surface state exist one time for each process, not one time for each rank. This matters for
the 20 GB total limit (accepted P6c run: 17.70 GB as sum of rank peaks in event 2).
Not in this design: locks for single cells or OpenMP loops over edits inside one zone.

### 5.9 Wall edits inside a pass (added 2026-11-15)

Source of the facts: a read-only helper read Bnd, BP, Dist and Eng on 2026-11-15. Status C
here means "read by the helper". The main agent verifies these lines in its source check.

Facts (C):
- A wall face belongs to one cell, so its owner is the owner of that cell (Dist.hpp:23).
  A wall point has no owner.
- The current wall state is one `BoundaryParameterState` for each rank, a full copy on each
  rank (BP.hpp:153-167, Eng:1205, Eng:3968): points with bindings, faces, index maps, face
  covers, feature traces, one `revision`. `AgreeParameters` compares the copies byte by byte
  only in `Directory::Build` (Dist:593, 1423-1436).
- An edit must carry the current revision (BP:645). `Prepare` reads the complete surface
  link of each vertex of a changed face, also faces outside the cavity (BP:754-760, 1171).
- Each `Publish` runs `PrepareParameters`: all ranks validate the union of all edits and
  vote; one conflict rejects the whole publication (Dist:1449-1629). Two reductions run also
  when nothing changed (Dist:1470, 1550).
- The new wall point is a function of the proposed point, the end-point bindings, the edge
  trace, the authority and the mobility policy (BP:1388-1415). The geometry proof of a face
  has no collective call. The mobility policy does not change (Dist:1504).
- `Clearance` tests each new cell against the cells of all ranks (Dist:441-562). With a
  mapped geometry it runs for each edit that changes the skin or a coordinate, also on flat
  markers (Eng:5508, Eng:1224). No local test exists.
- Some split variants grow their cavity through the directory during the edit
  (Eng:4522-4548, 5171-5214). `CorrectCollapse` is collective (Dist:836-887).

Design:
1. Local rule for a wall edit (stricter than 5.1): the cells of the cavity are cells of this
   rank, AND each wall face on each wall vertex of the changed faces is a face of a cell of
   this rank. Test: the replicated index `FacesOnVertex` against the owned cells. Then each
   record that the edit reads or writes can change only by this rank in this pass.
2. Inside a pass the rank commits the edit into ITS copy of the wall state, with the
   validation that exists (`Prepare` on the copy, BP:948-949). The revision is the revision
   of the copy. The rank adds the change (points, faces, covers, traces) to its change list.
3. A cavity can grow only into cells of the rank (local index). If it needs another cell,
   the edit is frozen for this pass. No directory call inside a pass.
4. At the end of the pass all ranks exchange the change lists one time and apply them in
   rank order. Lists of two ranks touch different points and faces (item 1), so the order
   does not change the result. Then `AgreeParameters` runs one time (it exists in the
   directory build). A merge conflict or a difference of the copies is an error of the
   method: stop and report. It is not a reason for a retry.
5. Body separation, local proof for one-point operators (plain split and star cone, the two
   operators of 4.2). Derivation: the new cells are the cones from the new point p over the
   inner skin faces S of the cavity. The insert proves that each of these cones is positive.
   Add the cones from p over the OLD wall faces W of the cavity. If these are positive or
   flat (exact orientation >= 0), then all cones from p over the closed surface S + W of the
   old cavity are not negative. So the old cavity is star-shaped from p, and the cones over
   S are inside it. The new cells are inside the old cavity, and no contact is possible.
   Cost: one exact orientation test for each old wall face of the cavity (two for a split).
   - Flat marker: p is on the plane of W, the test gives 0, the proof holds.
   - Convex body: a split point is on the fluid side of the chord, the proof holds.
   - Convex body, wall COLLAPSE: the new faces are chords inside the body. This first
     proof fails always. The same for a split on a concave wall. Item 5a covers these.
   An edit that fails this first proof gets the second proof of item 5a.
5a. Second local proof, for an edit that adds volume on the body side (added 2026-11-15
   after a user objection: the limit of item 5 for convex walls was not acceptable).
   Why the first proof fails there: a wall collapse on a convex body replaces faces by
   flatter faces. The new cells are the old cavity PLUS a thin sliver Q between the old
   wall faces W and the new wall faces. Q is a union of tetrahedra, each with one face on W.
   Q is on the body side of W, so it starts outside the mesh.
   Derivation: Q is connected and starts outside the mesh domain. Q can meet a cell only if
   a boundary face other than W crosses Q. So contact is possible only where another part
   of the wall is nearer than the depth of Q (a thin body, for example a sharp trailing
   edge). By the need rule of 4.1 the depth of Q is below the tolerance tau.
   Test, with the wall copy of the rank (no message):
   (T1) wall faces that share a vertex with the changed faces: exact test that each one
        meets Q only in shared vertices or edges (the exact predicate of `Clearance`);
   (T2) all other wall faces: none has a bounding box that overlaps the box of Q enlarged
        by the diameter of Q. A box tree over the wall faces of the rank copy is built one
        time for each pass and is read-only in the pass.
   (T3) the faces W existed at the start of the pass. If not, the edit waits one pass.
   Two ranks: let Q_A and Q_B be slivers of two ranks with a common point x. Then W_B is
   nearer to x than the depth of Q_B. If depth(Q_B) <= diameter(Q_A), the test T2 of rank A
   finds W_B (W_B is in the copy of A by T3, and it is not a neighbour face of A by item 1)
   and A does not apply its edit. In the other case the test of rank B finds W_A. So two
   slivers that overlap are never both applied.
   An edit that passes T1 to T3 is applied in the pass. Only an edit near a thin part of
   the body fails; it goes to the collective lane of 5.3 item 4 with the batch clearance
   that exists. Its count is a reported number (M9).
6. The wall state stays replicated in this design (5.6). Size on M6: about 150 B for each
   point and 600 B for each face (U, helper estimate), about 40 MB for each rank.

### 5.10 What S1 must not use inside a pass

`SplitSurfaceStrip`, `PlaceSplitPatch`, `ReconstructHeightStar`, `RefitWallSplit`,
`ReconstructWallSplit`, `ReconstructSplitPatch` (Eng:4581-5033) and `CorrectCollapse` grow
cavities with collective calls. They stay in the repair path of 4.2 (refused list), which
runs in the collective lane. S1 inside a pass uses `SplitEdge` and `InsertEdgeCavity` only.

## 5A. Stage S2: the rows, as engine operators in passes (added 2026-11-15)

This is the design section that step 8.6 asked for. Source of the facts: a read-only helper
read `SU2_CFD/include/adaptation/native_bl3d/*.inl` (X/), `CNativeBLPipeline3D.hpp` (P), Eng
and Bnd on 2026-11-15. Status C means "read by the helper".

### 5A.1 Facts

- Model (C): one column for each wall face. A prism (wall face, lid triangle at 1.3 to 2.2
  h0, lateral faces) is coned from an apex at h0; the apex star is the 8 cells of the prism
  (X/BLRebuild17.inl:11-13, X/BLFirstRow9:304-310). Registry `Rows`: lid for each wall
  vertex, apex for each wall face (X/BLFirstRow9:131-138).
- A row that is built in part is a conforming mesh (C): a vertex with no lift keeps its
  place and its lateral faces vanish (X/BLFirstRow9:354-358). In growth, stopped columns and
  growing columns are joined by fill cells (X/BLGrowth12:215-226).
- Local today (C): `PlanLift` / `KernelBound` (star of one vertex), `ApexLP` / `ConeColumn`
  (skin of one column), the row `Transaction` (at most 64 old and 128 new cells), the
  `LayerTransaction` (at most 64 cells, 8 to 16 column tops), `NextLayer` (point values of
  the metric along the column), `CheckFirstHeight` (one first child), the ramp rebuild of
  one column (apex star, ring of 4 cells).
- Global today (C): the one-rank guard (P:335, X/BLEvent15:188); a private copy of the whole
  mesh with its own indices and id counters (X/BLState.inl:68-107); file-scope caches and
  globals ("Serial, not reentrant", X/BLFirstRow9:6, 72-77); the direction field (search of
  the nearest constrained source over all sources, and each wall edit makes the whole field
  invalid, X/BLGrowth12:86-131, X/BLState.inl:286-290); the crease scan (X/BLFirstRow9:187);
  one global sort of the wall vertices for the row transactions (X/BLFirstRow9:1140); the
  growth front with one global layer index 0 to 47 (X/BLGrowth12:379-382); `MakeRoom`, one
  CG solve for each layer over the union of all regions that lack room, up to 400000
  vertices and 4000 iterations, committed as one move (X/BLDeform16:91-159); the completion
  step that builds a directory of the whole mesh for each candidate (X/BLEvent15:262-263);
  the audits (X/BLEvent15:23-62).
- The engine has a second first-row builder, for each wall cell, with one apex for each
  face and no lids (`CarveFirstLayer`, Bnd:582-663, Eng:2399-2641), and a third one for
  analytic walls that gathers all cells on rank 0 (Eng:2223-2385).

### 5A.2 Design

1. The wall does not change in S2. S1 gives the final wall (user rule: surface first). A
   wall face where no valid column is found is marked "row refused" and reported. It goes
   back to S1 as a wall need between two passes; S2 does not split the wall.
2. Data of a wall point that depends only on the final wall, the authority and the
   requirements is computed ONE time at the start of S2 and kept with the point: the
   direction (normal of the one-ring, blend with the nearest constrained source inside the
   blend radius, X/BLGrowth12:122), the crease distance, the marker roles. Each rank
   computes it from its copy of the wall state, so all ranks get equal values with no
   message. The global invalidation is not needed, because of item 1.
3. One first-row operator in the engine: the row `Transaction` of the serial stack (lift,
   `ApexLP`, `ConeColumn`, sectors), with its mathematics unchanged, as a cavity operator
   on the engine cell store. Its cavity is the star of the lifted vertices. Local rule 5.1
   (complete stars on the rank). Order: each rank sorts its own wall vertices by the same
   visibility key. The registry becomes row data of the wall cell (lid ids, apex id); it
   travels with the cell in a migration and in a checkpoint.
4. Growth with a LOCAL step rule that replaces the global layer index: a column can add
   layer k+1 only when each neighbour column has k layers or more, or has stopped by a stop
   rule (thickness, size match, layer cap, refusal). On one rank this keeps the difference
   between neighbour columns at one layer, as the global front does today. A frozen column
   at a rank interface holds its neighbours, so a ramp of one layer for each column forms.
5. Two passes complete the rows (user decision 4 of section 10). Pass 1: each rank builds
   all rows except a band at its interfaces; the band is at most as wide as the number of
   layers. Then the interface moves by that width plus the cavity margin, counted in rings
   of wall columns. Pass 2: the band is inside a rank and is completed. The new interface is
   in columns that are complete, so it has no work. Junctions of three ranks: a third pass.
6. `MakeRoom` runs on one rank: the same solve, on the regions of that rank that lack
   room, with zero displacement at each vertex that is not interior to the rank (the code
   refuses to move such a vertex already, Bnd:262-263). A region at an interface waits for
   pass 2. No message inside the solve.
7. Body separation in S2 (I, to verify in the source check): the row transaction and the
   layer transaction keep the skin of their cavity and attach the old cells to the new
   points, so the new cells are inside the old cavity. Then no `Clearance` is needed in S2.
8. The ramp rebuild (new h0) is one local edit for each column (split the column edges at
   the new lid, flip the lateral diagonals, rebuild the prism). Same local rule.
9. Completion and audits: the completion uses the local vertex index, not a directory of
   the whole mesh. Each rank audits the first heights of the wall faces it owns; one
   reduction gives the result. `CheckFirstHeight` reads one first child, so it is local.
10. Retired when the engine operator passes the row checks: the private mesh copy and the
    file-scope globals of the stack, the one-rank guard, `CarveFirstLayer` as a builder, the
    region build on rank 0, the refit variants of the wall split. All state of the new
    operator belongs to a worker (5.8).

### 5A.2a Rows between two adaptations (added 2026-11-15, user question)

Facts (C, helper): the serial stack writes a `RowRegistry` (lid for each wall vertex, apex
for each wall face, keyed by coordinates, P:684-703). The next event uses it for the
h0-ramp step when it is present (P:516, 636-663). The adapted mesh already has a side file
for the wall bindings (`*.su2.native_ref`).
Design (user decision 5 of section 10):
1. The row data is kept with the mesh: for each wall vertex its column (the chain of point
   ids), for each wall face its apex, and the certificate bit of 5B.2 item 1. It is row
   data of the wall cells (5A.2 item 3), written with the mesh.
2. It is never trusted blind. At the start of the next event each recorded column is
   checked on the mesh that was read: the points and cells exist, they form the column, and
   `CheckFirstHeight` passes for the requirement of NOW. A column that fails the check is
   dropped from the record; its cells are then ordinary cells.
3. It is never required. With no record (a mesh from another tool, a lost side file), the
   mesh is a plain mesh. The requirement checks decide: a wall face whose first cell fails
   the first-height check gets "rows needed", and S2 builds a row there with the same
   operator as in the first event. Cells that the new target does not want are removed by
   the general collapse of S3. No stage is chosen from the class of the input mesh.
4. What the record saves: unchanged columns are kept and not built again; `Unlift` (4.2)
   knows the column of a vertex; a change of h0 is one rebuild for each column (5A.2
   item 8); wall faces with a set certificate bit are not proved again.

On disk (C, the reviewer read P:59-66, 157-247 on 2026-11-15): the record of today is a
side file `<mesh>.native_rows` next to the adapted mesh, with a checksum and with the byte
count and hash of the mesh file. A missing file means "no record". A mesh file that differs
from the record refuses the record. It holds the first row (lid for each wall vertex, apex
for each wall face, overrides, extras). The design keeps this file and this binding, and
adds the upper layers of each column and the certificate bits. A fresh start from the
adapted mesh reads it when the file is beside the mesh; with no file, item 3 applies.

### 5A.2b A wall change edits the column, it does not destroy it (added 2026-11-15)

User objection: each adaptation can change the wall mesh, so the rows become invalid and a
record of them has no use. Answer of the design:
- How much of the wall changes in one adaptation is not measured (M14). The geometry need
  depends only on the wall shape, so it is stable after the first adaptation. The size need
  follows the flow solution, so it changes more in the first cycles than in the late ones.
- With the record, a wall edit can change the column with the wall, in place. These are
  the preferred operators; `Unlift` and a new row (4.2) are the fallback when one refuses:
  1. Column split, for a split of the wall edge (a, b): for each layer that both columns
     have, split the edge between the two column points of that layer with the general
     edge split, from the top layer down. Then split the wall edge, put the new wall point
     on the surface and move the new column points with it, so that the layer heights stay.
     Then rebuild the first-row cells of the four new wall faces with the column rebuild
     that exists (5A.2 item 8, `ApexLP`).
  2. Column merge, for a collapse of the wall edge: collapse the edge between the two
     column points of each common layer, top first, then the wall edge; rebuild the first
     row of the changed faces.
  3. Column move, for a move of a wall point: the column points move with it.
  Each step is one general edit with validity checks, and the mesh is conforming after
  each step. The cost of a wall edit is then about one small edit for each layer.
- Without the record these operators cannot exist: the code cannot know which points make
  a column, except by a guess from the shape of the mesh, which the architecture rule
  forbids. The other way without a record is to remove the complete BL in each adaptation.
- Worst case, the complete wall changes: with the record the cost is about the cost
  without it (all columns are edited or built again). In each other case it is lower.
- To confirm in the source check before step 8.6 (M17): the cell structure of the growth
  layers between two neighbour columns (lateral faces, fill cells), so that the split of a
  layer edge keeps the layer structure.

### 5A.3 Not designed yet

- Growth ratio: no separate check is needed (corrected 2026-11-15, user question). The
  requested growth is part of the target metric; a row step is one unit of that metric
  along the column (X/BLGrowth12:166-176). The final size gate measures the longest edge of
  each cell in the target metric (`size_bad`, Eng:799). To verify in the source check: the
  metric of the final gate contains the row demand.
- Facing walls: the stop of two fronts that meet comes from the composed metric (U). No
  rule is designed before the test case below shows what the present code does (user
  proposal 2026-11-15). Test "flat channel" (M12): a box; the bottom face and the top face
  are BL walls at distance G; the side faces are symmetry. Flat walls, so the curvature has
  no effect. Case 1 (control): requested thickness T with 2T < G; the fronts do not meet.
  Case 2: 2T > G; the fronts must meet. Run with the serial stack of today on 1 rank.
  Report for each case: valid mesh yes / no; first heights on both walls; layers for each
  wall; the stop rule that ended each column; the final q / J / size counts. The stop rule
  of the design follows from this result.
- The outer transition after the last row is ordinary S3 work (the stack has no closure).

## 5B. Stage S4: quality (added 2026-11-15; it was not designed)

Source of the facts: the accepted P6c run (M) and a read-only helper that read Eng, BP, AR
and Dist on 2026-11-15 (C = read by the helper).

### 5B.1 Facts

- (M) Event 1, rank 0, quality phase: 2165 s of 2942 s; 1274 batch rounds; 4945 accepted
  edits; 138,023 classification queries (28 for each accepted edit), 768 s inside them;
  48,053 candidate moves of wall vertices for 2,346 accepted moves, 42,327 refused by the
  shape rule; 4 complete surface proofs, 104 s; elections 242 s.
- (C) A trial is evaluated on one rank with no collective call (Eng:5446-5489; audited at
  Eng:5739). The collectives are for each round: slots, lookup, fetch, two reservations,
  `ClearanceBatch`, `Publish`, `measure` (Eng:3476-5960).
- (C) Operators (Eng:5302-5439): `MoveVertex` (3 steps), `OptimizedRepairPatch`
  (`MeshOptimizer`: `Smooth` / `SmoothBoundary`, `ShellBoundary`, edge removal, face swap,
  collapse), `CollapseVertex` or `Reconnect`. Cavity: a vertex star or an edge shell.
- (C) Acceptance of a trial: operator validity; `boundary_replacement_ready`; the shape rule
  (`ValidateMetricChange`: the hinge debt of q and J must fall, Metric3D:461-578); the
  guidance check `geometric_change` (Eng:4000-4031).
- (C) Where the exact geometry queries come from:
  (a) `geometric_change` asks ALL old wall faces of the cavity (magnitude) and ALL new ones
      (limit), also faces that did not change, with no memo (Eng:4017-4025). It rejects
      when the summed excess grows. About 13 faces for each trial.
  (b) `SmoothBoundary` asks the old star one time and each candidate that passes the shape
      rule (Eng:6637-6641).
  (c) `ShellBoundary` asks the new faces (Eng:6966); (d) `CoupledBoundary` asks the patch.
  (e) After each publication, `Selection::Add` asks the magnitude of EVERY wall face of
      EVERY new cell, changed or not (Eng:825).
  (g) `finish` runs the complete entity certificate on every rank (Eng:1945).
  Each query tries the cheap direct path first (74,820 calls in 5.9 s, M); the cost is in
  the union path. (a), (b), (d) and the unchanged faces of (e) only guard against a worse
  wall. Only (g), and (c) / (e) for new faces, decide validity.
- (C) `HasAcceptedFaceCertificate` (BP:1089) tests that a face, its points and its cover
  are identical to the accepted state. It stores no result. The census row has no value
  for each face (Eng:300-313).
- (C) `SmoothBoundary` builds up to 19 targets for each star. For each target it projects
  the point and measures q and J of the star BEFORE the shape rule (Eng:6542-6623).

### 5B.2 Design

Same two principles as S1: the estimate steers, the proof runs one time; each rank works
alone in passes.
1. Certificate bit. Each wall face of a cell has one bit in the census row: "proved inside
   tau". It is set only by an exact proof. It travels with the cell (migration,
   checkpoint). When an edit replaces a cell and a wall face keeps its three points and
   its binding, the new cell takes the bit of the old face (the identity test of BP:1089,
   inside the cavity). A new or changed face starts with the bit clear.
2. No exact geometry query for a trial. The acceptance of a wall-touching trial is:
   validity, the shape rule of today (it stays: the quality stage is the place for a
   progress rule), and the need rule of section 3 on each NEW wall edge: no new edge can
   have a geometry need. This replaces (a), (b), (c), (d). Cost: closed form, point data.
3. No exact query in `Selection::Add` (e). The geometry queue is filled by the need rule
   and by failed proofs, not by measured magnitudes of each new cell.
4. Proof at the end of S4, as S5: each rank proves its faces with a clear bit, one time
   (4.3). A face that fails is repaired by the loop of 4.3. After a repair, the points of
   the repaired faces are pinned for wall moves and wall swaps in the local quality work
   that follows, so the loop cannot undo itself. Volume edits stay free.
5. `finish` does not prove again a face with a set bit (subject to M15).
6. Passes. A quality trial is a local edit of 5.1 (5.9 item 1 when it changes the wall).
   The ladder of today runs unchanged on one rank, with the rank's own priority order by
   debt. No slots, reservations or `ClearanceBatch` for local edits. A move of a wall
   vertex uses the contact proofs of 5.9 items 5 and 5a: the added volume is the union of
   the tetrahedra (old wall face, new point) on the body side.
7. Wall-vertex moves: the order of the work for one candidate becomes cheapest first. The
   shape rule is tested on the target point before the projection where this is exact;
   where it is not, the order stays. No change of the target set before F3 shows where the
   4 ms for each candidate go (M16). This item is a measurement first, not a design.

### 5B.3 What this is expected to remove (I, not measured)

- Exact queries in the quality phase: from about 140,000 for each rank to the count of wall
  faces that changed since the last proof.
- The 4 complete surface proofs on every rank: one proof of the unproved faces by the owner.
- The collectives of each round: 1274 rounds become a small number of passes.
- Not removed: the compute of the trials. Its size is not known (M8).

## 6. Cost model (v1; each number M or A)

No-BL M6, first adaptation, 6 ranks. Build 293 took 3640.7 s (M).

| Part | Today (build 293, M) | Design | Basis |
|---|---|---|---|
| Collapse 2495 edits | 194.7 s | below 10 s | local edits; need rule; no per-trial proof (A) |
| Wall splits, about 12000 | 274.1 s (12222 edits, surface lane) | below 60 s | unit cost of a validity-only wall split not measured (M2) |
| Interior splits 70479 | 68.8 s | below 15 s | 1000 edits/s per rank (A, M4) |
| Quality 27480 edits | 2009.7 s | not predicted | compute share of this phase not measured; needs the new profile |
| Alternation and tails | 378.6 s | folded into passes | no global round for a few edits |
| Final proof | 674.9 s on each rank | about 190 s wall | 69544 faces x 16 ms / 6 ranks (16 ms = 520 s / 32618 calls, M); less with stored certificates |
| Pass overhead | - | about 30 s | 10 directory builds + 6 migrations (M unit costs) |

BL stage S1 from the saved 492 state: wall points predicted 8000 to 20000 (A, measure M1);
proof about 70000 faces. On 6 ranks: insert below 120 s (A), proof about 190 s. On 1 rank the
proof alone is about 1100 s, so S1 must run on all ranks (4.4).

Honest limits: the quality phase and the unit cost of a wall split are not measured. If the
measured numbers show that the full BL pipeline does not fit in 5400 s, the main agent
reports it with the numbers and stops. No larger budget.

## 7. Structure of the code

### 7.1 Layers

| Layer | Content | Shared by 2D and 3D |
|---|---|---|
| Core | cell and row storage, ownership, pass loop and exchange, migration, need queues, budgets and deadlines, checkpoints, stage sequence, driver, acceptance report | yes |
| Kernel (one for each dimension) | cavity operators, exact predicates, wall geometry, projection, wall proof, edge length, first-row operators | no |
| Requirements | size, geometry, features, wall rows, budgets | yes |

Kernel interface (what the core asks from a kernel): need(edge or vertex) from point data;
build the cavity of an edit and say if it is local; apply the edit with validity checks;
measure a cell (debts); prove a wall face; pack and unpack a cell with its row.
2D and 3D do not share operators or the wall proof.

### 7.1a Readable code (user order, 2026-11-15)

User: "make sure that the code implemented is human readable, well commented and not
spaghetti-like, no huge functions." The rules below are the reviewer's reading of this order.
They apply to all new code and to all code that a step of section 8 moves or changes. Code
that no step touches stays as it is until its step (no format-only commits in between).
1. The code passes the repository `.clang-format` (Google base, 120 columns). One statement
   on one line.
2. One function does one job. Limit: 80 lines for one function body. An exception needs a
   written reason in the report.
3. No large lambda inside a function. A lambda longer than 15 lines becomes a named function
   or a member of a named class. (`Adapt` today is one function of about 4800 lines made of
   lambdas; section 7.2 removes this.)
4. Each function, struct and file starts with a comment: what it does, its input, its
   output, and what must be true before and after. A comment gives the reason, not a copy of
   the code. A rule that comes from a derivation names the design section.
5. Names are words. One-letter names only for loop counters and for symbols that a comment
   defines.
6. One file holds one stage or one service of 7.2. No function gets a mode flag that changes
   what the function means; write two functions.
Check for each step: the report lists the 10 longest functions of the changed files with
their line counts, and the output of `clang-format --dry-run` on the changed files.

### 7.2 First cut of `Adapt` (identical results)

`Adapt` (Eng:1163-5971) is cut into these units, with the same behaviour and the same mesh:
Census (rows, debts, measure) - Queues (selection, priority) - Operators (calls into Bnd) -
Trial (validity and requirement checks) - Commit (apply, ids, metric transport, parameter
changes) - Exchange (directory, reserve, clearance, publication) - Control (phases, budgets,
deadlines, checkpoints) - Ownership (partition, migration).
Proof of identity: equal mesh hash and equal counters on the gates of section 8.0.

### 7.3 Requirements replace the switches

Each place that infers "a BL is present" from a non-empty container (about 150 in Eng, about
20 in Bnd; counts not verified) becomes one of three things: a check of a requirement; a rule
on a measurable property (for example "the face has geometry need"); or it is removed.
Examples: `PrepareSurface` runs with no wall rows (Eng:6011); batching does not depend on
"composition" or "curved" (Eng:3488-3495); budget shares come from the requirement record and
the cost model, not from `ADAP_BL_METHOD` (`CNativeRemesher3D.cpp:52`, :580-600).

## 8. Order of implementation (each step has a gate)

8.0 Gates of every step that touches shared code:
(a) no-BL M6 first collapse pass: work counters and accepted counts;
(b) BL stage-3 repeat of C492;
(c) 2D native tests, with and without BL, 1 / 2 / 4 ranks;
(d) at milestones: the full no-BL M6 workflow, both adaptations accepted in 5400 s.

| Step | Content | Results | Acceptance |
|---|---|---|---|
| 8.1 | Handoff steps 1 to 3: regression fix, cheap fixes, gates, timer tree, new profile | identical decisions | handoff 1.3 |
| 8.2 | Cut `Adapt` (7.2) | identical | gates a to c, equal hashes |
| 8.3 | Requirements record, stage sequence, geometry law in the shared target (2, 3, 7.3) | change | gate d; no-BL gets S1 |
| 8.4 | Stage S1: need rule, validity-only insert, certify by owner, S1 on all ranks (4) | change | wall ready and proved from `R/saved_surface492_checkpoint.json`, 1 and 6 ranks; gate d |
| 8.4a | Stage S4 with the certificate bit and the need rule in place of the per-trial exact queries (5B.2 items 1 to 5) | change | gate d; equal final gates; exact queries for each rank reported |
| 8.5 | Passes for volume edits, then for wall edits, then for quality edits (5, 5.9, 5B.2 item 6) | change | edits per second at 1, 2, 4, 6 ranks against 5.7; gates a to d |
| 8.6 | First row inside the engine: one construction for the fresh event, the leftover repair and the h0-ramp rebuild, valid at rank interfaces; then retire the serial stack and its three copies | change | BL first-row checks; BL on 6 ranks |
| 8.7 | Core out of 3D; 2D moves to the core and takes budgets, checkpoints and validity-first | change | gate c; user decision 1 of section 10 |

Why this order: 8.2 before any change of behaviour, so that later differences are real.
8.4 before 8.5 because the wall is the present blocker and S1 already runs on 6 ranks in
lockstep. 8.6 after 8.5 because the first row must be designed for rank interfaces one time.
Step 8.6 needs its own design section from the reviewer before code (the row mathematics
stays; the operator form and the column removal of 4.2 are open).

## 9. Numbers that the main agent must measure (no design work)

| # | Number | How |
|---|---|---|
| M1 | Wall points that the need rule asks for, on the Euler input mesh and on the saved 492 state | diagnostic that applies section 3 on the surface only, with `EvaluateSurface`; no production change |
| M2 | Unit cost of a validity-only wall split; share of plain split / cone / refused | counters and timers in a test build |
| M3 | Proof cost for each face, direct path against union path, on the accepted Euler 293 mesh and on the 492 state | existing query in mode `Classify` |
| M4 | Compute time of one interior edit on one rank | serial run of the interior lane |
| M5 | Cost of one projection (`BindNewPoint`) | timer |
| M6 | Share of wall splits whose new point is inside the old shell | exact test of 5.4 in the diagnostic of M2 |
| M7 | Share of edges with a complete local shell at 2, 4, 6 ranks | local test of 5.1 on the partitioned input |
| M8 | Compute against wait in the quality phase | new profile of handoff step 2 |
| M9 | Share of wall edits that pass the local rule of 5.9 item 1 at 2, 4, 6 ranks; share of wall edits that pass the local contact proof of 5.9 item 5, for splits and for collapses | the two tests as counters in the diagnostic of M2 |
| M10 | Rows on the smallest fixture where the serial stack reaches growth: layers for each column (largest, mean); count and size of `MakeRoom` regions; count of faces that need surface preparation in the completion; growth count of the vertex set in the row transaction | counters in the serial stack, no change of results |
| M12 | Test "flat channel" of 5A.3: two facing flat BL walls, cases 2T < G and 2T > G | new small fixture, serial stack of today, 1 rank; report the list of 5A.3 |
| M13 | Share of wall faces that fail the first exact proof after the need rule; largest E / tau | diagnostic of M1 followed by the exact query on the planned faces |
| M14 | On a mesh with rows: wall vertices that S1 must change in the next event; share of refused `Unlift` | counters; `Unlift` as a diagnostic on a saved mesh with rows |
| M15 | Source check, no number: can the entity certificate (`PatchErrorBounds`) be proved by patches of owned faces and combined (4.3)? | read BP:412-445 and AR:2131 |
| M16 | Quality phase: exact queries split by call site (a), (b), (c), (d), (e), (g) of 5B.1; time for one wall-move candidate split into export, projection, q / J, move, guidance | counters and the F3 timer tree; no change of results |
| M17 | Source check, no number: structure of the growth layers between two neighbour columns; does a split of one layer edge keep the layer structure (5A.2b)? | read X/BLGrowth12 `LayerTransaction` and X/BLRebuild17 |
| M11 | Source check, no number: the row transaction and the layer transaction keep the skin of their cavity (5A.2 item 7) | read X/BLFirstRow9 `Transaction` and X/BLGrowth12 `LayerTransaction` |
| M18 | Geometry cache need (handoff 2.0c): for each rank and event, the count of DISTINCT wall facets that the rank asks for, the mean bytes of one prepared facet, and the count of distinct scope keys beside the scope builds | one bit for each authority facet and a set of scope keys in `CurvedSearch`; print at the stage exit; no change of results |
| M19 | Source check, no number: in the final proof each rank makes the same count of geometry-bound calls (31,622 on rank 0 and on rank 4, F3 event 1). Does each rank prove the complete wall? Which data stops a proof of owned faces only? | read Eng:1945-2005 and BP:412-445 with M15 |
| M20 | Quality stage, private repair rounds: for each rank and window, `private_work_used` beside `busy_seconds`. Does one work unit cost the same time on each rank? Spread of busy time between ranks in one round (largest / mean) | existing counters at Eng:5794-5800; print the work beside the seconds; no change of results |
| M21 | Memory spike at the ownership transition (handoff 2.0e): for each rank, current RSS and peak RSS before and after the transition, beside the exchange bytes that the code knows (`max_exchange_work_bytes`, bytes sent). Which buffers make the rise of about 4 GB (event 1) and 2 GB (event 2) in the sum? | read `/proc/self/statm` and `ProcessPeakRSS3D()` at the stage boundaries around `ownership_staged`; print only |

## 10. Decisions for the user

1. Common acceptance contract for 2D and 3D (2D: q >= 0.18; 3D: q >= 0.2 and J >= 0.05).
   Option A (my pick): keep the two sets of numbers, share only the rule "construction is
   validity-first, the gates apply to the final mesh". No number changes. Option B: one set
   of numbers for both; this changes a gate of one of them.
2. When 2D moves to the core. Option A (my pick): last (step 8.7); the 3D M6 goal is not
   delayed, and 2D stays a regression gate. Option B: first, as a small pilot of the core;
   lower risk for the core, but about one step of delay for M6.

3. Threads (5.8). DECIDED by the user, 2026-11-15: option A.
   Option A: the design is ready for threads (worker rules of 5.8),
   the first implementation is MPI only, threads are a later step. Option B: MPI and threads
   are implemented together from the start; two new methods at one time, and an error is
   hard to assign to one of them.

4. Rows at a rank interface (5A.2 items 4 and 5). DECIDED by the user, 2026-11-15: option A.
   Option A: the local step rule
   and two passes with a wide interface move (about as many column rings as layers). Few
   exchanges; the order of growth is not the order of the serial code. Option B: one layer
   for each pass pair: all ranks grow one layer, exchange, the interface columns grow,
   exchange. The order is the order of the serial code and the interface move is narrow,
   but about 2 exchanges for each layer (about 100 for 48 layers).

5. Rows between two adaptations (5A.2a). DECIDED by the user, 2026-11-15: option A.
   Option A: keep the row record with the
   mesh, check each column at the start of the next event, and work as on a plain mesh
   where the record is absent or wrong. Option B: keep nothing; each event treats its input
   as a plain mesh. B is simpler, but each event then builds all rows again, and a change
   of h0 cannot use the column rebuild.

## 11. Open risks

1. The unit cost of a valid wall split and the operator mix are not measured (M2).
2. The proof of about 70000 faces can be the largest cost of S1 (M3).
3. Cells thinner than the sagitta at the wall: column removal is not specified yet (4.2).
4. Concave walls use the collective lane for body separation; slower (5.4).
5. Junctions of three or more ranks can leave a residue for the collective lane (5.5).
6. The quality phase is not modelled (M8).
7. The h0 ramp: the composed length of a wall chord at 2e-6 height can ask for a finer wall
   than the geometry law (annex question 12). Verify on one nose edge before step 8.6.
8. The estimate of section 3 uses point values. Where it is wrong, the proof finds it and the
   repair loop of 4.3 corrects it; the cost of that loop is not known.
9. Wall edits near a thin part of the body (sharp trailing edge) fail the second contact
   proof (5.9 item 5a) and use the collective lane. Their count is not measured (M9). The
   proof of 5a is new; the main agent checks it against the code before step 8.5.
10. The band that moves for the rows is wide (5A.2 item 5): about as many column rings as
    layers, with all cells above them. Its migration cost is not measured.
11. `MakeRoom` on one rank with fixed interface vertices can give less room near an
    interface than the global solve (5A.2 item 6). The count and size of these regions is
    not measured (M10).
12. A wall face with a refused column goes back to S1 (5A.2 item 1). The count of such
    faces is not measured (M10). If it is large, S1 needs a row need in its need rule.
