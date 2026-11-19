# Fast path for the native 3D adaptation: design choices (version 2, with the connection to the wall layers)

Author: reviewer, 2026-11-19. For the user. This is a design at the level of choices. It is not yet an order for the agent.
Data: Euler ONERA M6, 6 ranks, run `N4/section6_3_after_N11/round_8_5t_P/G5/full587` (event 1: 303k -> 726k cells, 2095 s).

## 1. Goal

One remeshing of this case in about 2 minutes on 6 ranks (now 33 minutes), with all final gates unchanged:
valid cells, size, quality (q >= 0.20, J >= 0.05), geometry 1e-4 on two sides, transfer, restart, two equal runs.
Not a goal: identical meshes to today. The fast path gives a different (valid, gated) mesh. It needs a new reference.

## 2. Where the 33 minutes go (measured)

| Part of event 1 (one rank, wall seconds) | Seconds | Note |
|---|---|---|
| Import, reference, partition | 45 | |
| Start census and first collapses | 226 | all ranks vote after each 10 cells |
| Splits on the surface | 260 | 147 ms for one accepted split |
| Splits in the interior | 51 | 4.6 ms for one accepted split; most of the changes, 2.6 % of the time |
| Exchange of cells between ranks | 31 | |
| Quality repair | 1310 | 88 % of its work is at the wall surface |
| Last size and geometry repair | 130 | |
| Final surface proof | 52 | was 122 before step 8.5t |
| Return of the mesh, transfer, output | 86 | |

Facts behind these numbers:

1. **The surface is repaired after it is built.** In the quality stage, 26,000 of 29,400 repairs are at the wall, and about
   21,000 of them change the surface triangles. The surface has 67,000 triangles. So about one third of the surface is built
   and then repaired. At the start of the quality stage, 98 % of the bad cells have a very short edge (below 0.5).
2. **Each attempt proves geometry with exact arithmetic.** Quality stage: 366 s for one rank in surface distance proofs. The
   proof of the complete final surface costs about 140 core-seconds. So the work proves the same surface many times, also for
   attempts that are then refused (only 17 % of the move attempts and 1 % of the reconnect attempts are accepted).
3. **All ranks go in step.** 24,390 rounds; about 200 "to all ranks" calls in one round; each rank proposes one change in a
   construction round. The addresses of the cells around a point are stored on a rank chosen by a hash, not on the owner. So
   also a change that is fully inside one rank talks to all ranks, and waits for the slowest rank.
4. **The mesh is stored three times in tree maps** (448 + 831 + 1300 bytes for each cell and rank). No neighbour table exists.
5. **Fixed parts are also slow:** output 35 s, return 31 s, final proof 52 s, start census 136 to 207 s.

Consequence: a faster round, or a fast pass only for the interior, cannot reach the goal. With all waiting removed, the
present design needs about 8 to 10 minutes.

## 3. The choices

### Choice 1: principle "fast work, one strict proof"
During the work: plain floating point, simple rules, no proof. At the end: the exact checks that exist today run ONE time on
the complete mesh (full measurement of all cells, two-sided surface proof). Places that fail go to the present strict engine
for repair. Cells are never invalid during the work (a volume test for each change; exact test only when the fast test is
near zero). So "validity first" and all final gates stay.

### Choice 2: a new lean local core, not a faster round
A new component (working name: local remesher). Each rank copies its cells into flat arrays with a neighbour table and does
all kinds of changes alone: collapse short edges, split long edges, reconnect, move points. The borders between ranks are
frozen in one pass. Then the ranks move the borders (the existing repartition) and repeat, about 3 to 5 times. The ranks talk
a few times, not 24,000 times.
Rejected: (a) faster rounds in the present engine: floor 8 to 10 minutes, and a deep change of the directory; (b) a fast pass
for the interior only: 2.6 % of the time; (c) an external remesher: against the purpose of the native engine.

### Choice 3: the lean core also does the surface
The gain is at the surface (facts 1 and 2). The core evaluates the reference surface in floating point (the cubic patches and
the point parameters exist in the reference data), places new surface points on it, keeps corners and feature lines, and
checks the distance with samples and a safety margin. The exact two-sided proof at the end decides. The surface is done first
(the rule "surface first" of the user), then the volume.

### Choice 4: the same metric rules as today
The core asks one "target" interface for all sizes (section 5, H2 and H3). Without wall layers (Euler): the length of an edge
is the closed formula of the present size gate (from the two end tensors), the tensor of a new point is the average of the two
end points (the present rule), and quality is q and J with the tensor at the cell centre. So the core and the final gate
measure the same numbers, and the core can aim at the gate values with a margin. With wall layers the present gate integrates
the target along the edge; then the core uses a cheap fixed rule for the same integral and the strict gate decides.

### Choice 5: the present engine stays as checker and repairer
The core runs before the present engine starts (place: `native_engine3d/NativeRemeshRun.inc`, before `SnapshotSource` in
`AdaptWorker`). The engine then finds a mesh that is almost done. Its strict census and proofs are the acceptance. Transfer,
output and restart do not change. If the core fails or is switched off, the engine does all the work as today.

### Choice 6: order of the work, each stage with its own measured gate
- Stage 0, measure (small): tests on saved real data. Speed of the lean kernels; cost of the copy into arrays; time that the
  present engine needs for a mesh that is already good (the fixed cost of the checker).
- Stage 1, the core for the volume, surface frozen. Small gain in time (about 10 %), but it builds and proves the base:
  arrays, new point numbers, tensors, frozen borders and cycles, same result in two runs, all gates.
- Stage 2, the surface in the core. This is the large gain (surface splits, wall repairs, last repairs: about 1700 s of 2095 s).
  It is the hard part and has the largest risk.
- Stage 3, the fixed parts: start census, final proof, return, output.
Steps N21 and N22 (in work, prepared) stay useful: the checker engine uses them (decision without the clock, census, shared proof).

## 4. What I expect, and what I do not know

| After | Event 1, forecast | Confidence |
|---|---|---|
| N21 + N22 (present lane) | about 28 to 30 min | good (measured parts) |
| Stage 1 | about 25 min | medium |
| Stage 2 | about 4 to 6 min (the fixed parts then dominate) | low: no measurement of our code yet |
| Stage 3 | about 2 min | low |

Not known: the real speed of a lean change in our data (stage 0 measures it); how many places fail the final strict checks
after the core (each failed place costs the old price); how well a float distance test predicts the exact surface proof; the
cost of the checker engine on a good mesh. Size: stage 1 is some days of agent work; stage 2 is some weeks. The wall layers
(RANS) are not built by stages 0 to 3, but the core has their connection points from the start (section 5).

## 5. Connection to the wall layers: the placeholders

Rule of the user (AGENTS.md): no Euler / RANS switch. So each step below is ALWAYS called. It reads the data (requested wall
heights, markers). With no requested wall height it gives its default, and that is the Euler path. "Today" says what the
present code does (source study of 2026-11-19).

The order is fixed. Each step uses only the steps before it.

| # | Step | Input | Output and what must be true after it | Default (no wall height) | Today |
|---|---|---|---|---|---|
| H0 | Wall data | markers, first height h0 (with the ramp value of this cycle), growth ratio g, thickness T, tolerance 10 % | one record for each wall marker; the same on all ranks; fixed for the event | empty | exists (`wall_heights`, `region_walls`) |
| H1 | **Wall surface target** (FIRST) | the reference surface (not a mesh), the solution sizes on the wall, H0, the crease rule | at each point of a wall marker: the sizes in the two directions along the wall. Along a sharp edge: the metric; across it: h0. Fixed for the event: no later scaling changes it | the solution sizes on the surface | no separate target in method METRIC; in FIRST_ROW it exists but is not made first |
| H1b | **Wall frame** (geometry only) | the reference surface with its sharp edges and curvatures, the other walls near it, H0 | at each point of a wall marker: a growth direction and a local thickness limit `T_loc <= T` (decision 5). Both change smoothly along the wall. The first height h0 holds everywhere; only the thickness is limited | not used | no such field; facing layers end where a change is refused, not by a rule |
| H2 | **Volume target** (made from H1 and H1b) | a point, the solution tensor there, H1 at the foot point on the wall, the growth direction and `T_loc` of H1b (in place of the plain normal and T), the distance d and the two curvatures of the wall at the foot point, H0 | ONE tensor. In the layer region: along the wall = H1 at the foot point CARRIED ALONG THE NORMAL to the distance d (decision 4: the sizes grow on a convex wall and shrink on a concave wall); normal to the wall = the progression (h0 at the wall, growing with g). Fade to the solution tensor between 0.9 T and T. Outside: the solution tensor. Check: at d = 0 its along-wall part is equal to H1 | the solution tensor | along-wall sizes come from the faces of the INPUT mesh, not from a surface target; the curvature is used only as an upper limit of the along-wall size, not to carry the size along the normal; a scaling step can change the volume target after the surface work |
| H3 | Length and quality rule | two points (or a cell), H2 | the length of the edge under H2 and q, J of a cell: the numbers that the strict gates use, or a stated cheap rule for them | closed formula from the end tensors | exists; with wall layers it is an integral along the edge (expensive) |
| H4 | Surface pass | the surface mesh, H1, the reference geometry | the surface mesh has the sizes of H1; geometry in tolerance; corners and feature lines kept | the same pass with the default H1 | surface and volume are refined together in METRIC |
| H5 | First row | the wall faces of H4, h0 | each wall face with a requested height has exactly one first cell; its top point is at h0 along the normal (10 %), with the foot inside the face; cells valid; surface not changed | nothing to do | exists for one cell at a time (`InsertFirstWallPoint`, `ConstructFirstLayer`) |
| H6 | Next layers (growth) | the first row, g, T | layers up to T with the ratio g; a closed outer skin; the layer cells are marked. Until this step is built: it does nothing, and the volume pass with H2 makes the growth (as today) | nothing to do | no layer step and no outer skin in the engine; no gate on the ratio |
| H7 | Protection rule for the volume and quality pass | a proposed change, H0, the marks of H5 and H6 | a change is allowed only if each wall face that passed its height check still passes; marked layer cells and the outer skin are not changed | all changes allowed | exists as checks inside the operators (`HeightCheck`, `PreservePassingFirstHeights`, constrained moves) |
| H8 | Volume pass | the mesh after H6, H2, H3, H7 | sizes and quality aim at the gates; surface and protected parts not changed | the Euler volume pass | generic changes that follow the target |
| H9 | Final wall gates | the final mesh, H0 | height check of each wall face (exists); ratio check of the layers (placeholder: no such gate today) | nothing to check | height check exists (`ProveFinalHeights`) |

Three decisions in this table:
1. **H1 before H2** (order of the user). The volume target takes its along-wall sizes from the surface target. Any scaling to
   reach the requested complexity is applied to H1 and H2 together, before the surface pass. Never after it.
2. **H2 uses the surface TARGET, not the built surface mesh.** The target is known before any mesh work and is the same on
   all ranks. The surface pass (H4) then follows it.
3. **The marks of the layers are new data.** Today the engine keeps no layer number and no outer skin; it finds the first
   cell again from the geometry each time. The core keeps a small mark for each cell (0 = free, 1 = first row, 2... = layer).

4. **The along-wall sizes follow the normals (curved walls).** The layers follow the wall normals. On a convex wall the
   normals open, so the distance between two neighbour normals grows with d. On a concave wall they close, so it shrinks.
   For a principal direction i of the wall with curvature k_i (positive = convex, as seen from the fluid):
   `size_i(d) = size_i(0) * (1 + k_i * d)`, with `size_i(0)` from H1. As a tensor: `M_t(d) = A^-T M_s A^-1`, `A = I + d * S`,
   where `M_s` is the H1 tensor and `S` is the shape operator of the wall at the foot point. This is the spacing that a layer
   with the same points in each row has. H2 does not enforce the surface spacing itself away from the wall.
   Limits that the contract states: (a) concave wall: the factor `1 + k_i * d` goes to zero at `d = 1 / |k_i|` (the normals
   meet). The layer region must end before that: the local thickness is limited to a fixed part of `1 / |k_i|`, and the fade
   starts there. (b) Sharp convex edge: the normals make a fan; the crease rule of H1 gives the size there. (c) Sharp concave
   corner: the normals of the two sides meet at once; H5 and H6 need their own rule there (open problem). (d) The solution
   sizes can ask for a smaller along-wall size than the carried one; then the smaller size is used.

5. **One wall frame field for all geometric conflicts (H1b).** The thickness of the layer is limited locally, and the growth
   direction can leave the wall normal, by rules that use the geometry only:
   - Sharp concave edge with the angle `theta` on the fluid side (the rule of the user, as in the Pointwise picture): for a
     wall point at the distance `s` from the edge along the wall, `T_loc(s) = min(T, c * s * tan(theta / 2))` (the layer stays
     on its side of the bisector plane; `c <= 1` leaves room for the transition cells). The growth direction turns away
     from the corner: at the edge it is the bisector direction, and it goes back to the wall normal with a smooth step
     over the length where `T_loc < T`. So the directions near the corner open like a fan, and the two walls share one
     direction at the edge.
   - Concave smooth wall: `T_loc <= c / |k|` (decision 4, limit a).
   - Another wall in front (a gap): `T_loc <=` a fixed part of the half gap.
   - `T_loc` is then made smooth along the wall with a limit on its rate of change, so the thickness has no sudden step.
   - Near the corner the volume target is the intersection of the targets of both walls: fine in both normal directions.
   The first height is never removed by H1b: each wall face keeps its first cell at h0 and its height gate.
6. **The solution inside the layer region: refine the layer, down to the surface. Do not stop the layer for it.**
   The surface target H1 at a wall point takes the smallest along-wall size that the solution asks for anywhere in the
   column above that point (along the growth direction, up to `T_loc`), carried back to the wall with the factor of
   decision 4. H2 then carries H1 up again. So a shock inside the layer region makes the whole column finer, from the wall
   to the top, with a smooth change to the neighbour columns (the size gradation rule along the wall). What the solution
   asks in other directions (normal to the wall, oblique) enters by the intersection with the solution tensor, as today.
   Not chosen: a local stop of the layer at a shock. Reasons: (a) the wall resolution is necessary under a shock foot, more
   than elsewhere; (b) the first-height gate holds for each wall face, and a stop needs an exemption of a final gate;
   (c) a stop needs a threshold "the solution asks too much", which is a tuned value; (d) the end of a layer is the sudden
   change that we want to avoid. The local thickness limit exists (decision 5), but only geometry sets it.
   Cost: more points under the shock, from the wall up. H1 stays before H2: it needs the solution sizes and the geometry,
   which are known before any mesh work.

7. **A column that is on more than one rank.** Three different cases:
   - The sizes (H1, H1b, H2). They do not need the column on one rank. The reference wall (its facets, edges and curvatures)
     is a copy on each rank today. H1 and H1b are fields on this reference wall, so each rank has the same arrays.
     The column rule of decision 6 is computed "from the volume down to the wall": each rank goes through ITS part of the
     solution field inside the layer region, finds the foot point on the wall (local, the wall is a copy), carries the
     requested along-wall size back to the wall and keeps the minimum at that wall place. Then ONE reduction (minimum) over
     the ranks joins the arrays. The values are minima of numbers in fixed wall directions, so the result does not depend on
     the order or on the partition. After that, each rank evaluates H2 at any point alone.
   - The work of the core without built layers (H6 empty, the layers come from the volume pass under H2). No column exists
     as an object. The cells at a border between ranks are frozen in one pass and are done after the border moves.
   - Built layers (H5, and H6 when it exists). Contract for the ownership: in the layer region, a cell belongs to the rank
     that owns the wall face under it (the wall face of its foot point). The partition is made on columns: one column
     (a wall face and all cells above it up to `T_loc`) is one unit with the weight of its cells. So a column is never on
     two ranks, and the borders between ranks in the layer region follow the growth directions.
     This rule holds ONLY while a step works on whole columns (H5, H6). After the layers are built, their cells are marked
     and protected (H7), so no later step needs a column on one rank. The next move of the borders (the core moves them
     between its passes in any case) is free again: by single cells, with weights for the expected work. The marks travel
     with the cells. If a later step works on whole columns again (a rebuild in the next adaptation, a smoothing of the
     layers), it asks for the division by columns again; one move of the borders costs about 10 s today for 700k cells.
     The checker engine after the core does not need this rule.

8. **The wall for all distances is the surface MESH, not the curved geometry (decision of the user, 2026-11-19).**
   - Reason: the flow solver sees the mesh faces. The first height and the wall distance of the solver are measured from
     the mesh face. Also: the surface mesh can be up to 1e-4 away from the curved geometry (the geometry gate), and a first
     height for RANS is about 1e-6. So a height measured from the curved geometry can be wrong by 100 first heights.
   - H5 and H6 build the columns on the ADAPTED surface mesh (it exists then, because the surface pass H4 is before them).
     The first height is measured from the flat wall face under the cell.
   - H2 measures its wall distance d from the adapted surface mesh too, when that mesh exists (each rank has a copy of the
     surface faces today). The step that is before the surface pass (the column rule of H1, decision 6) uses the reference
     wall: there an error of 1e-4 has no effect, because it only decides which wall place a request belongs to.
   - At the outer end of the layer the two walls give the same result (d is about T, much more than 1e-4).
   - H9: the height gate measures from the mesh face. NOTE: the present check on a curved wall also uses the certified
     distance to the curved geometry (it takes the larger of the two errors). That part goes away with this decision. It is
     a change of the definition of a final gate on curved walls, decided by the user here; the 10 % tolerance stays.
   - The curved geometry is still used for one thing: the surface gate (the surface mesh is within 1e-4 of the geometry).
   - Consequence for the next adaptation: where the surface pass changes wall faces, the columns on those faces are
     removed and built again (H5, H6 run after H4 in each adaptation).

What stages 0 to 3 build of this table: H0 to H4 and H8 with their defaults, and H7 and H9 as real calls of the checks that
exist. H5 and H6 are empty calls with a test that says "nothing to do without a requested wall height".

Open problems of the wall layers that these contracts do not solve (from the handbooks): no accepted RANS M6 mesh yet; the
height check measures from flat faces on curved walls; no gate on the ratio; the wake behind the trailing edge; a size jump at
the end of the fade; concave and mixed corners; memory of the first-row publication.

## 6. Next step if the user agrees

I write the order for stage 0 and the detailed design of stage 1 (data layout, the changes, numbering, cycles, gates), with a
source check, while the agent finishes N21 and N22.
