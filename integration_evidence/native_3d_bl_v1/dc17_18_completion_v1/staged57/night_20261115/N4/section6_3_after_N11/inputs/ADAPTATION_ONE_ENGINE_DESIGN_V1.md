# Design v1: one adaptation engine for no-BL, BL, 2D, 3D and MPI

Author: the reviewer (read-only session), 2026-11-14. Revision 2 of 2026-11-16 (section 0).
Status: revision 2 ACCEPTED by the user on 2026-11-16 ("I approve the changes"), with
steps 8.3, 8.4 and 8.4a for implementation after the cut. Accepted before: 8.2, 8.2b.
Not accepted yet: 8.5 (needs its source checks and M20), 8.6 (not ready), 8.7.
The main agent does not change this design. It checks it against the source, measures the
numbers of section 9, and implements after the user accepts.

Evidence annex: `M6_STAGE3_WALL_FIRST_DESIGN_BRIEF.md`, sections 3.1 to 3.9 (cited as "A3.x").
Two new read-only audits of 2026-11-14 are folded in (MPI layer; wall split and target).
Status letters: M = measured, C = read in the code, I = inferred, A = assumed (to be measured).

## 0. Revision 2 of 2026-11-16 (after the source check N3 and the measurements N5)

Inputs: `Handbook/ADAPTATION_DESIGN_SOURCE_CHECK_20261115.md` (14 points, cited as "SC n"),
`R/night_20261115/N5/reports/N5_measurement_table_draft.md` (cited as "N5"), and the two
full runs with the owner proof (P7). A paragraph that this revision changed starts with
"R2:". Text that R2 withdraws is deleted, not kept beside the new text.

| SC | What was wrong or open | Where it is corrected |
|---|---|---|
| 1 | The entity certificate cannot be combined from owner patches in the forward direction | 4.3, 5B.2 item 1 |
| 2 | A change of the proof or of the cache changes the partition weights of the next event | 5.5 item 6, 8 (rule R-det) |
| 3 | "Each split divides the error by 4" is an estimate, not a proof that the loop ends | 4.1, 4.3; 5.9 item 5a (no use of tau) |
| 4 | The row transactions keep their skin only when no carrier face is replaced | 5A.2 item 7 |
| 5 | `Unlift` as "remove apices, collapse column edges" is not the inverse of the row transaction | 4.2, 5A.2b |
| 6 | The wall state has one revision; change lists of two ranks cannot be applied as they are | 5.9 item 4 |
| 7 | The contact predicate that exists is tetrahedron against tetrahedron | 5.9 item 5a (T1) |
| 8 | The patch operators are local kernels; their ENGINE call paths are collective | 5.10 |
| 9 | `global_work` is a mixed counter, not one unit for each proposed edit | 5.5 item 4 |
| 10 | The ownership decision of today reads clock times | 5.5 item 6, 5.6 |
| 11 | Wrong words and counts in the facts of 5A.1 and 5B.1 | 5A.1, 5B.1 |
| 12 | The size gate is an upper bound; it does not prove the progression | 5A.3 |
| 13 | The migration has no entry with a destination list | 5.5, 8 (step 8.2c) |
| 14 | Memory: 12 GiB is the model limit of one rank; the job gate is 20 GB in total | 1 |

Measured facts that changed the design (N5):
- The need rule of section 3 is safe but too strict when it works alone. On the accepted
  mesh of build 293 ALL 54,961 wall faces pass the exact proof, and the need rule still
  marks 7,521 of 82,559 wall edges (9.1 %). So a proved face must overrule the estimate
  (4.1, R2).
- The owner proof works: the geometry work of the final proof fell from 418 s to 17 s for
  each rank (event 1) and from 521 s to 20 s (event 2). The events fell from 2683 s to
  2220 s and from 1977 s to 1451 s. All standing gates passed.
- 97.4 % of the edges have a complete local shell at 6 ranks; 96.8 % of the wall edges
  and 95.2 % of the wall vertices pass the local rule of 5.9 item 1 (M7, M9).
- In the quality stage a rank is busy for about 406 s and waits in the join for about
  508 s (event 1, mean of 6 ranks, M8). The compute of the wall-move candidates is about
  164 s for each rank (M16). The waiting is the cost, not the compute.

Short names: Eng = `Common/src/adaptation/CNativeEngine3D.cpp`, Bnd = `Common/src/adaptation/CNativeBoundary3D.cpp`,
BP = `Common/src/adaptation/CNativeBoundaryParameters3D.cpp`, AR = `Common/src/adaptation/CNativeAffineReference.cpp`,
BE = `Common/src/adaptation/CNativeGeometryBackend3D.cpp`, Dist = `Common/src/adaptation/CNativeDistributed3D.cpp`,
DH = `Common/include/adaptation/CNativeDistributed3D.hpp`, Part = `Common/src/adaptation/CNativePartition3D.cpp`,
MT = `Common/include/adaptation/CNativeMetricTransport3D.hpp`, Pipe = `SU2_CFD/include/adaptation/CNativeBLPipeline3D.hpp`,
BLM = `SU2_CFD/src/adaptation/CBoundaryLayerMetric.cpp`.

### 0.1 Revision 2.1 of 2026-11-16 (after two source inventories and the timer data)

These points win over the text below where they differ. The "how" is in
`IMPLEMENTATION_8_3_8_4.md`.

1. Section 3: the geometry law is a SERVICE that stages call (`EstimateWallEdge`). It is
   not put into the composed size target. Reasons: in the size target it would split
   wall faces that already pass the exact proof, and a non-empty composition switches
   other engine behaviour for the no-BL case. The curvature cap of the BL metric stays
   as it is for now; it uses the edge rule (constant 8, at metric length 1.8), the law
   uses the face rule (constant 6). To make them one rule is a later step with the rows.
2. Step 8.3 has IDENTICAL results: the law as production code with a print-only census,
   and the requirements record as a read-only view. Step 8.4 is the first step that
   changes a mesh, and its first part changes only the BL wall stage.
3. Section 4, the measured cause of the slow wall stage (serial, saved 492 state, 389 s):
   the guidance check `geometric_change` rejects 1,203 of 1,681 wall splits (72 %) and
   costs 167 s. It sums the squared excess of the faces with no area weight, so a split
   of one failing face into two failing faces counts as "worse". The first part of 8.4
   removes this veto for a needed and valid wall split. Another 2,117 trials are refused
   at the operators; their reasons are not counted yet, so no fix is designed for them.
4. Section 4.1: in the wall stage each new wall face gets its exact proof when its cell
   is committed (the query of call site (e) that exists). So the estimate does not
   decide the need of a new face there; it chooses the edge to split inside a failing
   face. The estimate alone judges only where no exact value exists yet.
5. Sections 4.3 and 5B.2 item 1, facts: the census row has no value for a face, it is
   not in the checkpoint, and it ends with the `Adapt` call. The place for a stored
   proof that travels with the cell is `OwnedCell::physical`. A query memo exists
   already and answers 50 to 67 % of the repeated queries. So the stored proof is
   designed only after its gain is measured.
6. Section 4.2, fact: `InsertEdgeCavity` does not project its point with `BindNewPoint`
   and refuses an edge between two canonical facets. Only `SplitEdge` binds the point.
7. Section 5B and step 8.4a: the numbers do not support this step now. The exact check
   of a quality trial rejects only 6.5 % (call site a) and about 11 % (call site b) of
   the trials, and an accepted trial needs the exact proof of its new faces anyway. The
   possible gain is about 25 s for each rank (1 % of the event). Step 8.4a is set
   aside. The cost of the quality stage is the waiting; that is step 8.5.

### 0.2 Revision 2.2 of 2026-11-16 (M20, and a source inventory of one quality round)

State: step 8.5 is NOT accepted and its implementation text is not written. This
section holds the new facts and what they change. Sources: M =
`R/night_20261115/section6_1/reports/M20_accepted_counters.csv` and
`current_profile_all_timer_rows.csv` (accepted run of work order 6.1); C = the file
`/tmp/claude-1000/-media-rausa-4TB-SU2-Versions-SU2-NativeIntegrated/f3d988e5-f14f-4a96-b6fb-963ec11730e7/scratchpad/inventory/ROUND_INVENTORY_20261116.md`.

Facts:

1. (M) Quality stage, private loop of each round (M20):

   | | Event 1 (1,274 rounds) | Event 2 (801 rounds) |
   |---|---|---|
   | Busy time of a rank, smallest / mean / largest | 344 / 416 / 484 s | 90 / 246 / 369 s |
   | Sum over the rounds of the largest busy time | 943 s | 589 s |
   | Mean wait of a rank in the join | 527 s | 343 s |
   | Model: one join for each 8 rounds | wait 327 s | wait 212 s |
   | Model: one join for each 64 rounds | wait 220 s | wait 188 s |
   | Model: one join for the stage | wait 68 s | wait 123 s |

   The model adds the measured busy times of N rounds for each rank and takes the
   largest sum. It is a lower bound of the wait for that join distance, not a promise.
2. (M) `private_work_used` does not follow the time. Correlation of work and busy time
   of a rank in a round: 0.38 (event 1), 0.47 (event 2). One unit costs 0.87 to 1.87 ms
   (event 1) and 0.88 to 2.60 ms (event 2) on different ranks. 36 % of the rank-rounds
   use 0 units and cost 0.16 s. The slowest 10 % of the rounds give 31 % of the sum.
   Which rank is the slowest changes: each rank is the slowest in 138 to 352 rounds.
3. (M) Quality stage of event 1, rank 0: 1,717 s. Join wait 523 s, communication 259 s
   (433,467 scopes, about 340 for each round), private busy time 407 s. So 782 s of
   1,717 s is wait and communication. Construction has 275 s of communication
   (4,845,975 scopes in 23,113 rounds).
4. (C) A round of the quality stage has 21 steps; table 1 of the inventory. The
   collectives of one round are in its table 2. The list of 5B.1 was not complete.
5. (C) Points 3 to 9 of the inventory section 9 correct this design: `AgreeParameters`
   runs only in `Directory::Build`, the cost of each round is `PrepareParameters`; the
   slots and the allowance need global values before the private loop; the id token
   needs a counter for each edit; the cavity of a private trial comes from Lookup and
   Fetch; the optimizer gets the remaining seconds.

What this changes:

a. The cause of the join wait is the difference of the busy times in ONE round, not a
   bad balance of the stage. The sum of the busy times of the ranks is nearly level
   (344 to 484 s). So a larger batch for each round helps only in part (wait 527 ->
   327 s for 8 times fewer joins), and a pass that is short and has the same count of
   edits on each rank keeps about 220 s. A long pass is necessary.
b. 5.5 item 4 changes. A pass share in work units, or in "edits tried", does not make
   the busy times equal (fact 2). New rule: a rank ends its pass when its local queue
   has no candidate left that it can try, or when its share of the global work budget
   is used (the budget is far from its limit: 1,023,830 of 46,492,288 units in event
   2). Both are counts, so rule R-det holds. No rule in seconds.
c. What stays after long passes is the imbalance of the stage: 68 s (event 1) and 123 s
   (event 2) for each rank. This is the job of the balance rule of 5.5 items 1 to 3. It
   comes after the passes, as its own step, with its own numbers.
d. Expected gain of passes in the quality stage (I, not measured): the join wait falls
   from about 520 s to about 70 to 120 s, and most of the 260 s of communication goes
   away. The private busy time (407 s) stays. The compute after the join (clearance
   97 s, census, publication) stays in part.
e. Step 8.5 is cut into parts, each with its own gate (proposal):
   - 8.5a: a local lane for edits that do not change the wall and whose complete cavity
     is on one rank (rule 5.1). No Lookup, Fetch, reservation, contact test or
     publication for these edits. The rounds of today stay for all other edits.
   - 8.5b: wall edits in the local lane (5.9: wall state merge, contact proofs).
   - 8.5c: the interface move (5.5), so that edits at the old interface become local.
   - 8.5d: the balance rule (5.5 items 1 to 3).
   Each part changes the mesh and gets new reference files by the rule of 2026-11-16.
f. Open before the text of 8.5a: how a rank reads a complete cavity from its own data;
   which edits of the quality stage need the contact test; what `Directory::Build`
   resets; the share of the quality candidates that are local and do not touch the wall.

## 1. What does not change

- All gates: geometry 1e-4, q, J, size, first height (10%), transfer, restart. Budgets: 5400 s
  for each event. Requested heights and progression. CAD stays optional.
- R2 (SC 14): memory. The job gate is 20 GB in total: the sum of the peak RSS of all ranks
  is at most 19073 MiB (user order 2026-11-15; option `ADAP_NATIVE_JOB_MEMORY_LIMIT`, P8).
  The 12 GiB option stays what it is in the source: the modelled limit of one rank.
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
Order: largest s(e) first (longest-edge bisection in the law metric).

R2 (SC 3): why the refinement ends. "Each split divides q by 4" is true for constant
curvature only; it is the estimate that says how many levels to plan. The proof that the
loop ends is this: (1) each new wall point is exactly on the surface; (2) bisection of the
longest edge makes the diameter d of the faces go to zero (known bound for longest-edge
bisection: d falls at least by the factor sqrt(3)/2 for each two levels); (3) on a
surface with bounded second derivatives (a cubic net has them) the distance between a
flat face with corners on the surface and the surface is at most (1/2) K d^2, with K the
largest curvature over the face. K is finite, but it is not computed. So the count of
levels is finite but not known before. The loop runs until the exact proof passes or the
budget ends. Example where the factor 4 fails: z = eps (x^3 - x) over the triangle
(-1,0), (1,0), (0,1); the midpoint of the long edge has z = 0, so the first split changes
nothing; the second level reduces the error. The angle terms of s(e) see this case (they
give eps at the end points, the true error is 0.385 eps).

R2 (N5, M1 and M3): a proved face overrules the estimate. The estimate is a sufficient
condition (constant 6, angle terms), so it marks edges whose faces are inside tau: 7,521
of 82,559 edges on a wall where all 54,961 faces pass the exact proof. Rule:
- A wall edge has a geometry need when s(e) > (3/4) tau AND one or two of its wall faces
  have no certificate (4.3, 5B.2 item 1). An edge between two proved faces has no
  geometry need.
- So S1 starts with the owner proof of the faces that have no certificate (cost on the
  accepted mesh: 252.7 s for 54,961 faces on one rank, 4.6 ms for each face, M3; about
  42 s on 6 ranks). Faces that pass get the certificate. Faces that fail give the first
  needs, with their measured error E.
- New faces (children of a split, faces of a wall collapse or of a wall move) have no
  certificate. For them the estimate decides, as written above. This is the safe side: a
  wall collapse that the estimate refuses is not done. Reported number (M23): wall
  collapses refused by the estimate alone. If the wall stays much finer than the target,
  a later version proves the candidate faces of one pass in one batch; not in v1.
- On a wall that is already inside tau, S1 makes no geometry split.

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
- R2 (SC 5): the operator `Unlift` of revision 1 is WITHDRAWN. The row transaction closes
  the stars of the lifted vertices over old apex stars again and again, makes sector
  points, gap cells, overrides and extras, and updates these records
  (X/BLFirstRow9:691-825, C). "Remove the apices, then collapse the column edges" is not
  the inverse of all these states. No general inverse is designed.
- Rule on data, not on a case label. A wall edit meets one of three cases at each wall
  vertex of the faces that it changes:
  1. No column record at the vertex: the edit is an edit on a plain mesh (operators above).
  2. A REGULAR column: the record of the vertex and of its wall faces has no sector, gap,
     junction, override or extra entry, and each neighbour column has the same layers
     with no fill cells between them. Then the column operators of 5A.2b apply (the
     column follows the wall). Their exact topology is not written yet (5A.2b).
  3. Each other column: the record of this column and of its wall faces is DROPPED. Its
     cells are then ordinary cells (the rule of 5A.2a item 2). The wall edit works on
     them as on a plain mesh. After the edit, the requirement checks decide: a changed
     wall face whose first cell fails the first-height check gets "rows needed"; S2
     builds the row with the same operator as in the first event; cells that the target
     does not want are removed by the general collapse of S3.
- Wall faces that S1 does not change keep their rows. A change of h0 only (the h0 ramp)
  uses the column rebuild of 5A.2 item 8.
- A wall edit that is refused in case 3 (the new point moves more than the thin cells
  permit, risk 3 of section 11) goes to the refused list below.
- Reported numbers (M14, new form): on a mesh with rows, the wall vertices that S1 must
  change; the share with a regular column; the share of refused edits in case 3.
- Retired only when cases 2 and 3 pass their checks in step 8.6: `RefitWallSplit`,
  `ReconstructWallSplit`, `ReconstructSplitPatch` and the growth loop of 12 growths
  (Eng:5003-5217), which re-fit apices under a wall split.

No new binding API is needed in v1. `BindNewPoint` re-projects a Cartesian point
(BP:1404-1407, C); the projected chord midpoint is used as today.

### 4.3 Certify (one time, same method for all faces)

- R2 (SC 1, M15, M19; measured with P7). The gate of today is the entity certificate:
  `GeometryCertificates` -> `PatchErrorBounds` (BP:412-445), with `complete=true` in
  `finish`. It has two directions. They are not equal for a split by owner:
  - Mesh to model (each mesh face is near the surface). This is a statement for each
    face, and the largest value over the faces is the value of the entity. So each rank
    proves the faces that it OWNS, and one MAX reduction gives the entity value. This is
    the costly part (31,622 union queries on each rank before). P7 does this
    (`OwnedFaceReverseBounds`; gate: the SUM of the owned proof faces is equal to the
    surface size). Measured: 418 s -> 17 s and 521 s -> 20 s for each rank.
  - Model to mesh (each part of the surface is near SOME mesh face). This is a statement
    about the complete entity; for patches it is "sup over the surface of the min over
    the patches", which is not the max of the patch values. It is NOT split by owner. It
    stays as it is in the source: the complete entity statement on the replicated wall
    state. No design step changes it.
- The certificate of a FACE (5B.2 item 1) stores only the first direction: "this face is
  inside tau, proved exactly". The entity statement of the second direction is computed
  again at each gate, from the complete wall.
- The certificate is stored in the census row of the cell, with the cell version (the row
  already holds `geometry_scores` by version, Eng:300, 863). Rows travel with migration
  and go into the checkpoint. A face that did not change is not proved again.
- A face that fails is repaired in a loop:
  1. The proof gives the measured error E > tau of the face.
  2. Plan n = ceil(log4(E / tau)) levels of splits (the estimate: the gap goes with the
     square of the edge length). Apply the n levels at once, with the insert of 4.2
     (longest edge in the law metric first).
  3. Only the new faces are proved. A new face that fails goes to step 1 with its own E.
  R2 (SC 3): n is a plan, not a bound. The loop ends by the argument of 4.1 (the diameter
  goes to zero, the curvature is bounded), and a collapse cannot undo a split (4.1). A
  split that both operators refuse goes to the refused list of 4.2. If the time budget
  ends with a failed face, the mesh is rejected; the gate does not change.
  Inside passes: the repairs of one proof run are the local edits of the next pass.
  Reported numbers (M13): share of faces that fail the first proof, and the largest E / tau.
- No sampling. No relaxed gate. The estimate of section 3 never accepts a mesh.

### 4.4 Where S1 runs

`PrepareSurface` is collective over all ranks already (C: the test fixture permits 6 ranks).
The one-rank limit comes from the serial first-row stack around it (Pipe:335). So S1 moves
out of `RunEvent` and runs on all ranks, also for a BL case. Until the first row is inside
the engine (8.6), the mesh is gathered on one rank after S1 for the serial row stack. This
needs a migration with an explicit destination, the same function as in 5.5. R2 (SC 13):
that entry does not exist today; step 8.2c makes it by an identical cut.

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

R2 (SC 5): "the complete cavity that its operator needs" is the cavity that the operator
really builds, with each closure step. For the row transaction this includes the closure
over old apex stars (X/BLFirstRow9:694-700), not only the stars of the lifted vertices.
R2 (N5, M7, M9): measured on the accepted mesh with the production partition: 99.3 %,
98.1 % and 97.4 % of the edges have a complete local shell at 2, 4 and 6 ranks. For wall
edges with the stricter rule of 5.9 item 1: 99.1 %, 97.3 % and 96.8 %; for wall vertices
(collapse): 98.6 %, 96.0 % and 95.2 %. So one pass can do about 95 % or more of the work
with no message, and the interface move (5.5) frees the rest.

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
should pass the local proof (A). A concave wall is correct but slower. The exact form of
the proof and its limits (wall collapses on a convex body fail it always) are in 5.9
item 5.
R2 (N5, M6): the assumption is NOT confirmed and not refuted. The diagnostic tested only
the first 32 wall edges in id order: 6 of 32 new points were inside the old cavity. A
sample of 32 edges with the lowest ids is not the wing (the reviewer expects that they
lie on feature lines, where the new point is ON the old faces; the proof of 5.9 item 5
accepts "flat", the diagnostic tested "inside"). Measure M6b of section 9 replaces it: all
82,559 wall edges, the exact orientation of the projected midpoint against the old wall
faces of the edge, counted as positive, zero and negative. If the share that passes is
low, the second proof (5.9 item 5a) carries the load and its cost matters; the design of
the passes does not change.

### 5.5 Moving the interface

Only a balance-driven ParMETIS partition exists (Part:334); no function moves an interface on
purpose (C). New small function: each rank marks its cells within K layers of an interface.
For each pair of neighbour ranks the band of one side goes to the other side (deterministic
rule on the rank numbers, alternating with the pass number). The old interface is then K
layers inside one rank and is free in the next pass. K = layers of the largest cavity + 1.
The migration that exists moves the cells, given a destination for each cell. R2 (SC 13):
`Repartition` takes weights and a part count, not a destination list
(`CNativePartition3D.hpp:36-39`); its body computes the colours and then uses them for
the exchange (Part:341-390). Step 8.2c cuts it into "compute the destinations" and
"migrate to given destinations", with identical results. The design uses the second part.
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
   R2 (SC 9): the pass share is counted in a NEW counter of the worker, "edits tried in
   this pass": +1 for each edit that the need rule (or the quality ladder) proposes and
   the worker tries. It is not `global_work`. `global_work` is a mixed counter (proposed
   counts at Eng:3810-3817, reserved optimizer work at 3846 and 5807, collapse correction
   at 3908, extra trials at 4230, 4660, 4891, 5073, 5185; limit max(4096, 64 x global
   cells), Eng:2135). It stays the global work budget, with its meaning of today; the
   workers reduce their use of it at each pass end.
   The load of a rank for the balance is: needs left x tried edits for each applied edit,
   measured on that rank in the last pass. These are counts, so the partition and the
   mesh do not depend on the host load.
   The size of the share needs M4; rule: the pass overhead (directory + migration, about
   5 s, M) must be a small part of the pass. No number is fixed here.
5. The band move changes the balance by K layers of one interface; the direction alternates
   with the pass number, so the drift is bounded by one band.
6. R2 (SC 2, SC 10): rule R-det, for each input of an ownership decision (balance,
   interface move, partition weights, pass share). The input is a count that has the
   same value (a) on a fast and on a slow host and (b) with a warm and with a cold
   geometry cache. Not permitted as inputs: seconds, and counters that a cache hit
   changes (`prepared_pieces` of the cache; `AtlasBudget::work` too, if the source check
   of step 8.5 shows that the reuse of a prepared facet changes it). Two places of today break this rule (C): the phase ownership reads elapsed,
   census, rebuild and cache-warming seconds (Eng:2933-3030); the partition weights of
   the next event read `prepared_pieces` of the cache (AR `StructuralEstimate`,
   `CNativeImport3D.cpp` 1762-1806), and Eng:2957 reads it inside an event. The second
   one made the event-2 mesh change when the final proof changed (N1). Until step 8.5
   replaces these rules, they stay as they are (the cut 8.2 must not change them), and
   each fix that changes the cache content or the proof order gets new reference files
   (user decisions of 2026-11-16). Step 8.5 removes both inputs: the geometry term of a
   weight is the predicted piece count from the control net alone (the `predicted` value
   that exists, `CNativeImport3D.cpp` 1662-1663).

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
today, C). R2 (SC 10): for one rank count the result of the DESIGN is deterministic by
rule R-det (5.5 item 6). The source of today has no such rule; the full runs of
2026-11-15 and 2026-11-16 gave the same files each time, which shows that the clock
inputs did not bind on this case, not that they cannot.

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
4. R2 (SC 6): the merge at the end of a pass, with one common revision.
   - All ranks start the pass with the same wall state at revision R (proved by
     `AgreeParameters` at the last pass end).
   - Inside the pass the revision of a private copy counts the edits of that rank. It is
     a private stamp: it rejects a stale edit on this rank (BP:645), nothing else.
   - At the pass end each rank sends its NET change against the state R: the records
     (points with bindings, faces, covers, feature traces) that it removed and the
     records that it has now. Not the list of its edits, and not their revisions. Net
     changes of two ranks touch different records (item 1), so their union is one
     well-defined change of the state R.
   - Each rank applies the net changes of the OTHER ranks to its copy as data. The owner
     validated each edit with `Prepare` on its copy; the other ranks do not validate
     again. This needs one new entry of the wall state: "apply the records of another
     worker". A record that is already changed on the receiving rank is a broken
     item 1: error of the method.
   - Then all ranks set the revision to R + 1, and `AgreeParameters` compares the copies
     byte by byte, one time (it exists in the directory build). A difference is an error
     of the method: stop and report. It is not a reason for a retry.
   - To verify in the source check of step 8.5: the revision is used only to reject stale
     edits and is not part of a value that must follow the edit history; the index maps
     and the face covers can be rebuilt from the records.
5. Body separation, local proof for one-point operators (plain split and star cone, the two
   operators of 4.2). Derivation: the new cells are the cones from the new point p over the
   inner skin faces S of the cavity. The insert proves that each of these cones is positive.
   Add the cones from p over the OLD wall faces W of the cavity. If these are positive or
   flat (exact orientation >= 0), then all cones from p over the closed surface S + W of the
   old cavity are not negative. So the old cavity is star-shaped from p, and the cones over
   S are inside it. The new cells are inside the old cavity, and no contact is possible.
   R2 (SC, T-table): the premises are: S + W is the complete boundary of the old cavity,
   with the right orientation and with no self-intersection (true for a cavity of valid
   cells), and the new cones over S are positive (proved by the insert).
   Cost: one exact orientation test for each old wall face of the cavity (two for a split).
   - Flat marker: p is on the plane of W, the test gives 0, the proof holds.
   - Convex body: a split point is on the fluid side of the chord, the proof holds.
   - Convex body, wall COLLAPSE: the new faces are chords inside the body. This first
     proof fails always. The same for a split on a concave wall. Item 5a covers these.
   An edit that fails this first proof gets the second proof of item 5a.
5a. Second local proof, for an edit that adds volume on the body side (added 2026-11-15
   after a user objection: the limit of item 5 for convex walls was not acceptable).
   Why the first proof fails there: a wall collapse on a convex body replaces faces by
   flatter faces. The new cells are the old cavity PLUS a thin sliver between the old
   wall faces W and the new wall faces. The sliver is on the body side of W, so it starts
   outside the mesh.
   R2 (SC 7): the cover Q of the sliver, in a form that the existing predicate can use.
   The three operators here have ONE new or target point p in each new wall face (split:
   the new point; wall move: the new place; wall collapse of v onto u: p = u). Q is the
   set of the tetrahedra (p, f) for each old wall face f of the changed set with a
   non-zero exact orientation, in both directions, each stored with positive orientation.
   Why Q covers the sliver: take a point x of the sliver. The old faces and the new faces
   form a closed surface, and x is inside it. The new faces are cones from p, so the ray
   from p through x cannot cross them; so it crosses an old face f beyond x, and x is in
   the tetrahedron (p, f). The tetrahedra on the fluid side are not needed for the cover;
   they are kept because they only make the test stricter.
   Derivation of the test: the sliver is connected and starts outside the mesh domain. It
   can meet a cell only if a boundary face other than W crosses it. R2 (SC 3): revision 1
   said here "the depth of Q is below tau". That does not follow from a point estimate
   and it is not needed. The test uses the real diameter of Q.
   Test, with the wall copy of the rank (no message):
   (T1) R2 (SC 7): wall faces that share a vertex with the changed faces and are not
        changed. By item 1 the owner cell of each such face is a cell of this rank.
        - Owner cell outside the cavity: `ConformingDisjoint(owner cell, t)` for each
          tetrahedron t of Q (`CNativeMesh3D.hpp:67-69`; both arguments are positive
          tetrahedra, as the predicate requires; shared nodes are equal by id). A wall
          face that Q crosses is a face of its owner cell, so Q then enters or overlaps
          that cell and the predicate refuses.
        - Owner cell inside the cavity (the edit replaces it and the face stays): the
          new cell on that face contains p, and the operator proved that it is positive.
          So p is on the fluid side of the face. No more test.
        The test is exact and on the safe side.
   (T2) all other wall faces: none has a bounding box that overlaps the box of Q enlarged
        by the diameter of Q. A box tree over the wall faces of the rank copy is built one
        time for each pass and is read-only in the pass (it is new; the `CollisionIndex`
        of today is a tree of owned volume cells, Dist:398-409).
   (T3) the faces W existed at the start of the pass. If not, the edit waits one pass.
        The rank keeps the keys of its wall faces at the pass start (new, local).
   Two ranks: let Q_A and Q_B be covers of two ranks with a common point x. Then x is
   nearer to a face of W_B than the diameter of Q_B. If diameter(Q_B) <= diameter(Q_A),
   the test T2 of rank A finds that face (it is in the copy of A by T3, and it is not a
   neighbour face of A by item 1) and A does not apply its edit. In the other case the
   test of rank B finds a face of W_A. So two covers that overlap are never both applied.
   An edit that passes T1 to T3 is applied in the pass. An edit that fails goes to the
   collective lane of 5.3 item 4 with the batch clearance that exists. The box test is on
   the safe side, so it can also refuse an edit that is far from a thin part of the body.
   The count of refused edits is a reported number (M9; not measured yet).
6. The wall state stays replicated in this design (5.6). Size on M6: about 150 B for each
   point and 600 B for each face (U, helper estimate), about 40 MB for each rank.

### 5.10 What S1 must not use inside a pass

R2 (SC 8): `SplitSurfaceStrip`, `ReconstructHeightStar`, `PlaceSplitPatch`,
`RefitWallSplit`, `ReconstructWallSplit`, `ReconstructSplitPatch` are local patch
operators (Bnd:1677, 1896, 1991, 2188, 2274, 2398; no directory argument). What is
collective is the ENGINE path that calls them (Eng:4581-5033): it gets and grows their
cavities through the directory. `Directory::CorrectCollapse` is collective itself
(DH:127-130). In v1 these engine paths stay in the repair path of 4.2 (refused list),
which runs in the collective lane. S1 inside a pass uses `SplitEdge` and
`InsertEdgeCavity` only. A later version can call a patch operator inside a pass when
its cavity is local; not in v1.

### 5.11 Part 8.5a: the local lane (added 2026-11-16, revision 2.2; NOT accepted yet)

Sources: section 0.2; C = `inventory/ROUND_INVENTORY_20261116.md` (R) and
`inventory/LOCAL_LANE_INVENTORY_20261116.md` (L) in the scratchpad of the reviewer,
commit `0acacc8cc8`. This section replaces 5.2 and 5.3 for the edits that it names. The
rounds of today stay for all other edits.

Idea. Before the slots of a global round, each rank works alone on the candidates of its
repair queue that only its own cells can see. It applies each accepted edit at once to
its own cells. When all ranks have no such candidate left, they join one time, the
directory gets the net change in one publication, and the round goes on as today.

L1. Lane candidate. A feature (vertex, edge or face) of the repair queue is a lane
    candidate when:
    (a) its cavity is complete on this rank: each face of a cavity cell that contains
        the first vertex of the feature is shared by two cells of this rank (the rule
        that `Directory::LookupImpl` uses for a star, Dist:658-680, with local data);
    (b) no cell of the cavity has a physical facet (`OwnedCell::physical`, DH:20-31).
    Why (a) is exact with (b): with no physical face the star of the vertex is a closed
    ball, and its cells are connected through faces that contain the vertex. If one
    cell were on another rank, one of these faces would have no second local cell.
    New code: one function that builds the star from `Selection::vertex_cells` and
    `working` (as the lambda `feature_cells`, Eng:567-585) and makes this face test.
L2. Trial. The ladder of today (`evaluate_candidate` with `private_repair`), unchanged.
    Its inputs `Dependencies` and `Imported` (DH:34-59) are filled from `working`:
    `{cell, version, this rank}` and the rows themselves. The cap of 4,096 cells for a
    coordinate star stays. Allowance for each trial: 160, as today when the work limit
    is far.
L3. Commit condition. The approved patch is applied in the lane only when
    `SameSkin(old, fresh)` is true (Eng:233), the parameter edits are empty and the
    coverage delta is empty (Eng:6024-6031). If not, the feature stays in the queue for
    the slots of the round. Nothing is applied.
L4. Local commit. The same steps that a commit of today does on the data of the rank
    (L section 5, items 3, 5, 6, 10, 12, 14, 16): new rows with `PublishMetrics`
    sensors, erase and insert in `working`, `Selection::Drop`, the measure of the new
    cells with local votes and local imports, `Selection::Add`, `Service`, `Touch`.
    The rank writes each removed ORIGINAL row and each added id in a journal.
L5. No contact test in the lane. Reason: positive cells that conform and keep the skin
    of the cavity fill the old region exactly (L3 tests the skin; the operators test
    the cells). Today each approved proposal goes through `ClearanceBatch`
    (Eng:5950-5961); in the accepted run it rejected none (ladder `rejected[5]` = 0).
L6. No reservation and no receipts in the lane. Two ranks cannot have a cell, an inner
    face, an inner edge or an inner vertex of a lane cavity in common, because the
    cavities are complete on one rank and their skins do not change. A lane candidate
    is in the queue of one rank only.
L7. New ids. The token formula stays (Eng:4082-4087), with a new counter `id_round` in
    the place of `round`. `id_round` grows with `round`. In a lane the edit number j of
    a rank uses the epoch `(id_round + j / batch) * size + rank` and the slot
    `j % batch`. At the join, `id_round` grows by the largest `ceil(n / batch)` of the
    ranks. `round` itself does not change (the window rule reads it, Eng:3611).
L8. End of a lane for a rank: no lane candidate is left, or its share of the global
    work is used (`(work_limit - global_work) / size`), or its own clock says that the
    event deadline is reached. No rule in seconds for the balance (rule R-det).
L9. Join and publication, one time for each lane: one reduction (work used, commits,
    largest edit count, failure, deadline); then the journals go into the directory by
    `Directory::Publish` with a new mode "the rows are already applied" (it skips the
    two local checks Dist:1601-1605, 1610 and the local erase / insert Dist:1626-1630;
    `Prepare`, `Commit`, the contact index `added`, `NotifyChanges` and
    `NotifyRepairChanges` run as today). A lane with no commit on any rank sends
    nothing.
L10. Not in the lane: a cavity with a physical facet (part 8.5b), a cavity that is not
    complete on one rank (part 8.5c frees it), all edits while a coarsening checkpoint
    is valid, the construction stage (its own text after the quality stage works).

What is new code: the star function of L1, the local fill of L2, the journal, the mode
of `Publish`, the counter `id_round`, the lane loop with its join. All other functions
are called as they are. After the cut (8.2) the commit steps of L4 are named functions;
the lane calls them. So 8.5a comes after the cut.

Gates (the mesh changes; new reference files by the rule of 2026-11-16):
- unit tests: the star function against `LookupImpl` on a partitioned fixture (same
  cells, and "not complete" for each interface feature); the journal publication gives
  the same directory as a rebuild (`Directory::Build`) of the same cells;
- one rank: the lane takes each candidate with no physical facet; the final gates pass;
- 2 / 4 / 6 ranks: all standing gates of the full two-event run, two runs with the same
  files; report the join wait, the communication seconds, the rounds, the lane commits
  and the lane seconds for each rank beside section 0.2;
- the standing safeguards for shared code (C492 by G3 rule 1, 481, 2D at 1 / 2 / 4
  ranks).

One number is missing before the text for the agent: M24 (section 9). It says which
share of the quality work is a lane candidate. If the share with a physical facet is
the larger one, 8.5a alone gives a small gain, and 8.5a and 8.5b are built as one step.

Risks: (1) the order of the edits changes (each rank follows its own queue), so the
mesh changes; (2) a long lane holds the interface candidates back until the join;
(3) the ids grow by the largest lane, not by the sum (L7); (4) the argument of L5 is a
proof for cavities with no physical face only; (5) the helper did not read each line of
Eng:4478-5440 and the body of `OptimizeMetricMesh`: the source check of the item must
confirm that no trial of a lane candidate calls the directory.

## 5A. Stage S2: the rows, as engine operators in passes (added 2026-11-15)

This is the design section that step 8.6 asked for. Source of the facts: a read-only helper
read `SU2_CFD/include/adaptation/native_bl3d/*.inl` (X/), `CNativeBLPipeline3D.hpp` (P), Eng
and Bnd on 2026-11-15. Status C means "read by the helper".

### 5A.1 Facts

- Model (C; R2, SC 11, words corrected): the first row has one PRISM for each wall face
  (wall face, lid triangle at 1.3 to 2.2 h0, lateral faces), coned from a face apex at
  h0. In the regular case the apex star is the 8 cells of the prism
  (X/BLRebuild17.inl:11-13, X/BLFirstRow9:304-310); sector and extra-skin cases differ.
  A COLUMN of the growth is for each wall VERTEX, started from `rows.lift`
  (X/BLGrowth12:339-346). Registry `Rows`: lid for each wall vertex, apex for each wall
  face, and the sector records (X/BLFirstRow9:131-138). In this design "column" means the
  chain of points above a wall vertex; "prism" means the cells above a wall face.
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
7. Body separation in S2. R2 (SC 4, M11): the row `Transaction` and the
   `LayerTransaction` keep the wall faces and the artificial faces of their cavity, but
   they can REPLACE carrier faces (faces on another boundary marker along which a column
   runs) and then check only the boundary loops of that marker (X/BLFirstRow9:774-802,
   X/BLGrowth12:230-269). So three cases, decided on the data of the transaction:
   (a) the complete oriented skin is unchanged: the new cells are inside the old cavity;
       no `Clearance`.
   (b) carrier faces are replaced, and all old and new points of these faces are in one
       plane (exact orientation equal to zero against three of them), and the loops are
       equal: the new faces cover the same flat region; the new cells are inside the old
       cavity; no `Clearance`. This is the symmetry-plane case.
   (c) carrier faces are replaced on a carrier that is not flat: the contact proofs of
       5.9 items 5 and 5a with the changed carrier faces in the place of the wall faces;
       if they fail, the collective lane.
   `ClearAbove` of the growth is another thing: it answers a refusal for inverted cells
   (X/BLGrowth12:209-214, 293-326); it is not the distributed `Clearance`.
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
4. What the record saves: unchanged columns are kept and not built again; a wall edit
   knows if the column of a vertex is regular (4.2, R2); a change of h0 is one rebuild
   for each column (5A.2 item 8); wall faces with a set certificate bit are not proved
   again.

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
  the preferred operators; case 3 of 4.2 (drop the record, plain mesh, then a new row) is
  the fallback when one refuses:
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
- R2 (SC 5, M17): status of this section: NOT READY for code. The source check found:
  at a front that is built in part, the `LayerTransaction` makes fill tetrahedra in
  sequence (X/BLGrowth12:215-226); a general split of a complete shell keeps the mesh
  conforming, but it does not update the layer chains or the identity of the fill cells;
  the ramp rebuild handles only its own case (first-row lid split, diagonal flip, prism
  rebuild, X/BLRebuild17:10-23, 189-267). So the three operators above are defined only
  for REGULAR columns (4.2, case 2), and also there the reviewer must write the exact
  mapping of the records (which lid, apex and layer ids exist after each step) before
  step 8.6. Each other column uses case 3 of 4.2 (drop the record, plain mesh).
- The sentence "without the record these operators cannot exist" stays true. The gain of
  the record is now limited to regular columns; its size is not measured (M14).

### 5A.3 Not designed yet

- Growth ratio. R2 (SC 12): the statement of revision 1 "no separate check is needed" is
  WITHDRAWN. Facts (C): the target contains the wall-normal profile (BLM:2377-2383), and
  a row step is one unit of that target along the column (X/BLGrowth12:154-176). So the
  requested progression is BUILT by the row operator. But no gate PROVES it: the final
  size gate is an upper limit for the edge length (`size_bad`, Eng:819, 827); a cell that
  is too short passes it, and it does not compare two layers. The design keeps the row
  mathematics and adds no gate. It adds a reported number (M22, print only, step 8.6):
  for each column, the ratio of the heights of two layers that follow each other against
  the requested growth; the largest difference and its place. A gate for the progression
  is a user decision (decision 6 of section 10).
- Facing walls: the stop of two fronts that meet comes from the composed metric (U). No
  rule is designed before the test case below shows what the present code does (user
  proposal 2026-11-15). Test "flat channel" (M12): a box; the bottom face and the top face
  are BL walls at distance G; the side faces are symmetry. Flat walls, so the curvature has
  no effect. Case 1 (control): requested thickness T with 2T < G; the fronts do not meet.
  Case 2: 2T > G; the fronts must meet. Run with the serial stack of today on 1 rank.
  Report for each case: valid mesh yes / no; first heights on both walls; layers for each
  wall; the stop rule that ended each column; the final q / J / size counts. The stop rule
  of the design follows from this result.
  R2 (N5, M12, measured with the serial stack, 1 rank, gap G = 1): control (T = 0.4):
  3,734 positive cells, 50 columns, 15 layers, 136 transactions, 20 refused; `MakeRoom`
  15 calls. Fronts that meet (T = 0.6): 3,914 positive cells, 50 columns, 17 layers, 142
  transactions, 110 refused; `MakeRoom` 17 calls. In both cases all cells are positive
  and the first heights pass, and both have q / J / size debt (control: q 280, J 278;
  meet: q 400, J 406), so neither is an accepted mesh. Reading of the reviewer (I): where
  two fronts meet, the code of today ends the columns by REFUSED transactions (110
  against 20), not by a rule; the mesh stays valid, and the debt rises. A stop rule on
  data is still to design, after the per-column stop reasons of the two cases are read
  (`R/night_20261115/N5/stack_measurements.md`). It is not needed before step 8.6.
  Observation for the user (M12, M10): each measured first height is 0.9 x h0 (0.0036
  for 0.004; 0.0054 for 0.006), which is the lower edge of the 10 % gate on each face.
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
- (C) `SmoothBoundary` builds targets for each star: 19 in the fallback (Eng:6620-6627),
  15 in the search along a feature edge (Eng:6606-6609), up to 24 in the old branch for a
  chart interior (Eng:6611-6618) (R2, SC 11). For each target it projects the point and
  measures q and J of the star BEFORE the shape rule (Eng:6542-6623).
- R2 (SC 11): (e) is narrower than written above: `Selection::Add` asks the wall faces
  only when extension components exist, with the marker filter of the surface planning
  (Eng:838-845).
- R2 (N5, M16, the clean full run of 2026-11-16, quality stage of event 1, sum of the 6
  ranks): union queries by call site: (a) 306,208 calls, 546 s; (b) 235,088 calls,
  1,005 s; (e) 145,793 calls, 140 s; (c) 168 calls, 1 s; (d) none. The seconds are the
  time inside the calling function and they overlap in part. Together about 282 s for
  each rank; the timer tree gives 244 s for each rank as "geometry bound" in the quality
  stage. Wall-move candidates: 325,324 in event 1 (3.03 ms each, 985 s as the sum of the
  ranks, about 164 s for each rank) and 122,811 in event 2 (3.87 ms each, 475 s). The
  export is 0.05 ms of a candidate. The function time of call site (b) is as large as
  the candidate time, so the candidate time is mostly these queries (the scopes overlap).
- R2 (N5, M8): in the quality stage of event 1 a rank is busy in its windows for about
  406 s and stands in the join for about 508 s (sums 2,437 s and 3,051 s over 6 ranks).
  After P7 the event is about 2,200 s for each rank: communication about 690 s, join
  wait about 540 s, geometry bound in the quality stage about 245 s.

### 5B.2 Design

Same two principles as S1: the estimate steers, the proof runs one time; each rank works
alone in passes.
1. Certificate bit. Each wall face of a cell has one bit in the census row: "proved inside
   tau". It is set only by an exact proof. R2 (SC 1): the bit means the mesh-to-model
   bound of this face (the part that P7 proves by owner, 4.3). The model-to-mesh
   statement of the entity is not stored; each gate computes it again from the complete
   wall. The bit travels with the cell (migration, checkpoint). When an edit replaces a cell and a wall face keeps its three points and
   its binding, the new cell takes the bit of the old face (the identity test of BP:1089,
   inside the cavity). A new or changed face starts with the bit clear.
2. No exact geometry query for a trial. The acceptance of a wall-touching trial is:
   validity, the shape rule of today (it stays: the quality stage is the place for a
   progress rule), and the estimate of section 3 on the NEW wall edges. R2 (N5, M1): the
   estimate marks 9.1 % of the edges of a wall that is inside tau, so the rule "no new
   edge can have a need" would stop valid wall moves in the curved zones, where the
   quality debt is. The rule is: the trial passes when no new wall edge has
   s(e) > (3/4) tau, OR the largest s(e) of the new wall edges is not above the largest
   s(e) of the old wall edges of the changed faces. This is the "not worse" rule of (a)
   of today, with the closed-form estimate in the place of the exact magnitude. The new
   faces get a clear bit, so the exact proof of item 4 sees each of them one time.
   This replaces (a), (b), (c), (d). Cost: closed form, point data.
3. No exact query in `Selection::Add` (e). The geometry queue is filled by the need rule
   and by failed proofs, not by measured magnitudes of each new cell.
4. Proof at the end of S4, as S5: each rank proves its faces with a clear bit, one time
   (4.3). A face that fails is repaired by the loop of 4.3. After a repair, the points of
   the repaired faces are pinned for wall moves and wall swaps in the local quality work
   that follows, so the loop cannot undo itself. Volume edits stay free.
5. `finish` does not compute again the mesh-to-model bound of a face with a set bit. It
   computes the entity statement of the other direction each time (R2, SC 1). Because of
   rule R-det (5.5 item 6): a stored bit changes which facets the cache prepares, so this
   step gets new reference files, as P7 did.
6. Passes. A quality trial is a local edit of 5.1 (5.9 item 1 when it changes the wall).
   The ladder of today runs unchanged on one rank, with the rank's own priority order by
   debt. No slots, reservations or `ClearanceBatch` for local edits. A move of a wall
   vertex uses the contact proofs of 5.9 items 5 and 5a: the added volume is the union of
   the tetrahedra (old wall face, new point) on the body side.
7. Wall-vertex moves. R2 (N5, M16): the measurement is done. A candidate costs 3.0 to
   3.9 ms, and nearly all of it is the exact queries of call site (b); the export is
   0.05 ms. So item 2 (no exact query for a trial) is the fix for the candidate cost, and
   no change of the target set or of the order is needed. This item is closed.

### 5B.3 What this is expected to remove (I, not measured)

- Exact queries in the quality phase: from about 115,000 union queries for each rank
  (687,000 over 6 ranks, M16) to the count of wall faces that changed since the last
  proof. Time: about 245 s for each rank in event 1 (M).
- The complete surface proof on every rank: done by P7 (M: 418 s -> 17 s, 521 s -> 20 s).
- The collectives of each round: 1274 rounds become a small number of passes. This is
  the largest part: communication about 690 s and join wait about 540 s for each rank
  in event 1 (M, after P7). How much of it the passes remove is not measured; the
  compute that stays is about 406 s of busy windows for each rank (M8).

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

R2: state of the model after N5 and P7 (no-BL M6, 6 ranks; times for each rank).

| Part | Measured now | Design | Basis |
|---|---|---|---|
| Event 1 / event 2, total | 2220 s / 1451 s (P7 run; before 2683 s / 1977 s) | not predicted as a sum | M |
| Final proof, geometry | 17 s / 20 s (before 418 s / 521 s) | done | M (P7) |
| Proof of a wall face | 4.6 ms mean; union path 8.0 ms, direct path 0.08 ms | owner proof of 55,000 faces: about 42 s on 6 ranks | M3 |
| Plain wall split, operator only | 1.7 ms (32 samples, all plain, none refused) | 12,000 splits: about 20 s of compute in total | M2, small sample |
| Projection of one point | 0.05 to 0.29 ms (3 samples) | not a cost | M5, small sample |
| Geometry queries of the quality stage | about 245 s | removed by 5B.2 items 2 and 3 | M16 |
| Communication | about 690 s (event 1) | passes; size of the gain not measured | M |
| Join wait | about 540 s (event 1) | passes with shares in counts (5.5 item 4) | M; M20 pending |
| Busy windows of the quality stage | about 406 s | stays | M8 |
| Local share of the work at 6 ranks | 97.4 % of edges, 96.8 % of wall edges | basis of the passes | M7, M9 |

Not measured, so the model has no number for them: the cost of one interior edit (M4:
one refused sample only), the cost of a wall split with its cavity and metric work in
the engine, the share of faces that fail the first proof after the need rule (M13).

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
| 8.2 | Cut `Adapt` (7.2); plan `CUT_PLAN_8_2.md`; accepted by the user 2026-11-15 | identical | gates a to c, equal hashes |
| 8.2b | Cut `CNativeRemesher3D::Remesh`, `MeshOptimizer`, `Selection` (work order item N6); accepted | identical | the gates of the cut plan |
| 8.2c | R2 (SC 13): cut `Repartition` into "compute the destinations" and "migrate to given destinations" (Part:341-390) | identical | the gates of the cut plan |
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

R2: what is ready and what is not, after the source check.

| Step | State of the design | Open before code |
|---|---|---|
| 8.2, 8.2b, 8.2c | ready, accepted (identical results) | nothing |
| 8.3 | ready | the implementation text (reviewer) |
| 8.4 | ready with the R2 texts of 4.1 and 4.3; the owner proof is done (P7) | the implementation text; M6b |
| 8.4a | ready with the R2 texts of 5B.2 | the implementation text |
| 8.5 | ready in principle; three new parts need the source check that R2 names: the merge of the wall state (5.9 item 4), the cover Q and test T1 (5.9 item 5a), rule R-det (5.5 item 6) | M20 (work against time for each rank), M4, the implementation text |
| 8.6 | NOT ready: the column operators (5A.2b), the stop rule for facing walls (5A.3) | a new design section |
| 8.7 | not started | user decisions 1 and 2 |

Reference files (R2, rule R-det): steps 8.3 to 8.7 change the mesh. Each of them gets
new reference files, by the rule that the user accepted for P7 and B7 on 2026-11-16: all
standing gates pass, and two runs with the same binary give the same files.

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
| M14 | R2: on a mesh with rows: wall vertices that S1 must change in the next event; the share with a regular column (4.2 case 2); the share of refused edits in case 3 | counters on a saved mesh with rows, in step 8.6 |
| M15 | Source check, no number: can the entity certificate (`PatchErrorBounds`) be proved by patches of owned faces and combined (4.3)? | read BP:412-445 and AR:2131 |
| M16 | Quality phase: exact queries split by call site (a), (b), (c), (d), (e), (g) of 5B.1; time for one wall-move candidate split into export, projection, q / J, move, guidance | counters and the F3 timer tree; no change of results |
| M17 | Source check, no number: structure of the growth layers between two neighbour columns; does a split of one layer edge keep the layer structure (5A.2b)? | read X/BLGrowth12 `LayerTransaction` and X/BLRebuild17 |
| M11 | Source check, no number: the row transaction and the layer transaction keep the skin of their cavity (5A.2 item 7) | read X/BLFirstRow9 `Transaction` and X/BLGrowth12 `LayerTransaction` |
| M18 | Geometry cache need (handoff 2.0c): for each rank and event, the count of DISTINCT wall facets that the rank asks for, the mean bytes of one prepared facet, and the count of distinct scope keys beside the scope builds | one bit for each authority facet and a set of scope keys in `CurvedSearch`; print at the stage exit; no change of results |
| M19 | Source check, no number: in the final proof each rank makes the same count of geometry-bound calls (31,622 on rank 0 and on rank 4, F3 event 1). Does each rank prove the complete wall? Which data stops a proof of owned faces only? | read Eng:1945-2005 and BP:412-445 with M15 |
| M20 | Quality stage, private repair rounds: for each rank and window, `private_work_used` beside `busy_seconds`. Does one work unit cost the same time on each rank? Spread of busy time between ranks in one round (largest / mean) | existing counters at Eng:5794-5800; print the work beside the seconds; no change of results |
| M21 | Memory spike at the ownership transition (handoff 2.0e): for each rank, current RSS and peak RSS before and after the transition, beside the exchange bytes that the code knows (`max_exchange_work_bytes`, bytes sent). Which buffers make the rise of about 4 GB (event 1) and 2 GB (event 2) in the sum? | read `/proc/self/statm` and `ProcessPeakRSS3D()` at the stage boundaries around `ownership_staged`; print only |

### 9.1 Results of 2026-11-16 (R2)

Source: `R/night_20261115/N5/reports/N5_measurement_table_draft.md` and the source check.
The full values and their limits are there; this table says what each row means for the
design.

| # | Result | Meaning for the design |
|---|---|---|
| M1 | Need edges by the estimate: 7,521 of 82,559 (accepted mesh 293), 6,759 of 59,884 (saved state 492), 4,368 of 58,305 (Euler input) | the estimate is strict; a proved face overrules it (4.1) |
| M2 | 32 edges: all plain splits, 1.7 ms each | small sample; the operator is cheap |
| M3 | 293: all 54,961 faces pass, 252.7 s; 492: 36,861 pass, 2,992 fail, 95.0 s | proof cost 2.4 to 4.6 ms for each face; the union path is the cost |
| M4 | one refused attempt only | NOT measured |
| M5 | 0.05 to 0.29 ms, 3 samples | not a cost |
| M6 | 6 of 32 | not usable; M6b replaces it |
| M7 | 99.3 / 98.1 / 97.4 % at 2 / 4 / 6 ranks | passes can do nearly all work locally |
| M8 | busy 406 s, join 508 s for each rank (event 1, quality) | waiting is the cost |
| M9 | local rule: 96.8 % of wall edges, 95.2 % of wall vertices at 6 ranks; the contact-proof share is NOT measured | 5.9 item 1 is workable |
| M10 | no complete growth on the small fixtures (the stack stops on a dependency guard) | NOT measured |
| M11 | answered: the skin is kept only with no carrier change | 5A.2 item 7 |
| M12 | both cases valid, both with debt; 20 against 110 refused transactions | 5A.3; stop rule open |
| M13 | NOT measured (needs a planned set of S1 faces) | measured in step 8.4 |
| M14 | NOT measurable (`Unlift` is withdrawn) | new form in 4.2 |
| M15 | answered: no combination by patches in the forward direction | 4.3 |
| M16 | (a) 306k, (b) 235k, (e) 146k union queries in the quality stage of event 1; candidates 3.0 ms | 5B.2 items 2, 3 and 7 |
| M17 | answered: fill cells exist; a general split does not update the layer records | 5A.2b not ready |
| M18 | done (accepted run of 6.1): prepared-facet builds = distinct facets, 102,866 in event 1; the need of a rank is 82 to 130 MB, below the pool of 175 MB | the cache fix B7 is withdrawn (handoff 2.0c) |
| M20 | done: section 0.2 | long passes; no share in work units |
| M21 | exported (`R/night_20261115/section6_1/reports/M21_accepted_counters.csv`); not analysed yet | memory spike |
| M19 | answered: each rank proved the complete wall | done by P7 |

New rows:

| # | Number | How |
|---|---|---|
| M6b | All wall edges of the accepted mesh: exact orientation of the projected midpoint against each old wall face of the edge shell; counts of positive, zero, negative; the same for the collapse of each wall vertex onto its nearest wall neighbour | diagnostic only; no split is done |
| M22 | For each column: ratio of the heights of two layers that follow each other against the requested growth; largest difference and place | print only, in step 8.6 |
| M23 | Wall collapses that the estimate alone refuses (the faces would pass the exact proof) | counter in step 8.4, with an exact proof of a sample of the refused candidates |
| M24 | Lane census (5.11), quality stage and construction stage, for each rank: for each candidate that reaches its private trial, the class "all cavity cells on this rank" (yes / no, from `Dependency::owner`) x "a cavity cell has a physical facet" (yes / no); for each class the trials, approved patches, commits, busy seconds and optimizer work; for the commits of each class the counts of `SameSkin` true, empty parameter edits, empty coverage delta | print only, under `options.timings`; no change of results; work order 6.7 |

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

6. R2 (SC 12): a gate for the growth ratio. No gate proves the requested progression
   today; the row operator builds it. Option A (my pick): no new gate; print the number
   M22 in step 8.6 and decide with it. Option B: add a gate now; this is a new acceptance
   rule with a tolerance that somebody must choose.

7. R2: reference files for steps that change the mesh. DECIDED by the user, 2026-11-16,
   for P7 and for B7: all standing gates pass, and two runs give the same files; these
   files are the new reference. The design uses the same rule for steps 8.3 to 8.7.

## 11. Open risks

R2, new risks:
13. Rule R-det is new. Until step 8.5 the ownership decisions read clock times and the
    cache content (5.5 item 6). A fix that is "identical" by its logic can change the
    files of the next event. Each such fix needs the two-run check.
14. The need rule is strict. With "a proved face overrules it" S1 does no extra split on
    a good wall, but wall collapses and wall moves are judged by the estimate alone and
    can be refused without need (M23). The wall can stay finer than the target asks.
15. The first contact proof (5.9 item 5) can pass much less often than assumed (M6: 6 of
    32, not representative). Then the second proof carries the load, with its box test.
16. The merge of the wall state (5.9 item 4) needs a new entry of the wall state and a
    source check of the revision. An error there breaks the replicated state; the byte
    comparison at each pass end finds it, but only after the pass.
17. The column operators for a wall change are defined for regular columns only, and
    their record mapping is not written (5A.2b). Each other column falls back to "drop
    the record, plain mesh"; how often that happens is not measured (M14).

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
