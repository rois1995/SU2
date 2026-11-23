# FP2-D, continuation 1: an edge that carries an error ends the path of step D

Author: reviewer, 2026-11-23 (design review and source check by Codex). For the main agent. The user sends it; it
is a user order. Copy this file with its SHA256 into `R/night_20261115/FP2_D/inputs/`.
Code state: the checkpoint of the failed lab, `d5e8ec972a` (candidate of build656, kept) = S0 of this
continuation. "The order" = `FAST_PATH_STAGE2_D1_REV2.md` (SHA256 `69178513...d8f1`). This text continues the
order; where the two differ, this text wins; section 4 lists what no longer holds. The reference of the lab stays
the lab of FP2-C part 2 (build655), as the order says. The helper-watchdog rule of the order holds. Short names as
in the order; `Fixtures` = `native_local_distance_fixtures.inc`.

## 1. Why

Your lab trial N1 (build656) failed one gate: `sweeps_distance` 40 (limit 39). Your diagnosis is correct: no
wiring error; the fault is in the design of the order. What your rows show:
- the splits of step D for each sweep: 670, 697, 718, 494, 257, 88, 64, 37, 49, 30, 36, 25, 33, 24, 32, 24, 32,
  then 22 and 32 in turn up to sweep 40; in the late sweeps each mark was split and none was deferred;
  `waits_neighbor` 55,131; `path_max` 184; 1,176 faces left `Limit`;
- 3,914 of the 3,926 splits are edges of the table Err.
So the paths of the pooled key put the requests of many faces onto few end edges: the bottleneck is in the
MARKING, before the stamps, the refusals or the order of the marks. One explanation is a chain of faces along the
span of the wing nose, each with two error edges and each pointing to its neighbour; the rows establish the
bottleneck, they do not locate the single paths. No more measurement is necessary before the change.
The path came from FP2-B, for the LONGEST-edge choice: a face must not be cut again and again through an edge
that does not carry its error (the halving chain that made the tiny edges). An edge of the table that is not a
feature edge has a sampled midpoint error above the limit on at least one face at it. To mark it without a walk
to farther faces removes the serial order of independent error cuts.
What your trial also shows (keep it in the report): the final surface has 50,222 faces in place of 75,954, with
3,926 splits of step D in place of 17,123. The choice of the edge does what the order wanted.

## 2. The change (production: one rule in `FollowDistancePath`, its declaration, its two calls, its comment)

With `distance_error_cut` on: an edge that is in the table Err of the sweep and is NOT a feature edge ENDS the
path. In `Mesh::FollowDistancePath`, between the lock block (the refused / border / ring-border checks and the
floor check of the present edge, which stay where they are with their condition `steps != 0 || check_first`) and
the walk over `SurfaceParents`: if the edge is in the table and `geometry.Feature` of its id pair is false, return
it as the end of the path, not locked, without a look at the other face. Pass `const SurfaceGeometry&` to the
private helper (update the declaration and the two calls: in `ChooseDistanceEdge` and in the marking of the
feature parents). For every other edge (not in the table, or a feature edge) the walk is as in the order. With the
option off nothing changes. The key is not changed, the length of the walk gets no limit, and no second mark is
added.
What follows (no other code change is intended; if one is necessary, write it in the R4 list):
- an ordinary request that passes its seed checks and whose chosen edge is a nonfeature edge of the table marks
  that edge directly: zero steps, no wait. The feature edge with a chord above the limit still comes first, and
  the first edge keeps its checks of today (seed refusal, border set, `Short`; a first edge with a border ring is
  still refused by `SplitSurface` with `Border`);
- a nonfeature error edge can cut the face on its other side (a face that did not ask for it) through an edge that
  is not that face's chosen edge, also when that face's own chosen edge is locked. An ASKING face still never
  takes another edge in place of its own locked choice;
- a FEATURE edge keeps the rule of the order: it is split only when it is the chosen edge of each of its parents.
  The path does not stop at a feature edge of the table; for the feature parents (`check_first = true`) the
  refusal, the border ring and the floor of the other edge are checked BEFORE the stop, and a locked other edge
  still holds the feature request back;
- the counters keep their meaning: `waits_neighbor` = asking faces (feature requests included) that wait for
  another mark; `path_max` = real steps of a walk, also of a locked one; `path_splits` = accepted marks with no
  direct request (also an end of a feature-parent walk with zero steps); `d_marks_error` / `d_marks_longest` =
  accepted splits in / not in the table. Stamp deferrals are apart;
- the locks, the reasons, the seed test `Short`, rule 2 of `SplitSurface`, the table Err and `ChosenSurfaceEdge`
  do not change.
What the change does NOT promise: the stop keeps the floors of each edit and rule 2, but it changes which faces
can be cut. The pooled sample is a heuristic (the larger of two samples that start at each face's own facet, also
values that did not converge): it gives no proof that an error falls after a cut and no bound on the sweeps.
(On a smooth curve that the mesh resolves, the distance of a chord falls about four times with each halving; that
is the reason to expect that repeated cuts of one edge end.) So the gates of the lab decide: the metric-edge gate,
the altitude of the last pass, `star_max`, the sweeps. Take the anisotropy and the count of sweeps as measured
results.
Replace the comment at the path by: "A nonfeature edge of the table has a sampled midpoint error above the limit
on at least one face at it. It is marked without a walk to farther faces, so that independent error cuts are not
put in a serial order (lab N1 of build656: the walks put the requests onto few end edges, 40 sweeps). The pooled
sample is a heuristic: no decrease and no sweep count is guaranteed; the length floor and rule 2 stay. A feature
edge still needs each parent to choose it. When the walk goes on, the key of an edge is the same from each face at
it and each step goes to a strictly larger key, so the walk ends before the guard."

## 3. Fixtures and the probe

3.1 The fixtures of the order whose expected results used the old path. Expected by the review (verify each by
hand in a test comment against the real rings and lengths; pin your derivation; R13 for a difference):
- (d2) `SkewedBox(false)` with the three fake midpoint samples: sweep 1 marks 0-2, 2-3 and 3-6. The order by
  length is 3-6, 2-3, 0-2. Their rings share cells, so one split in each of three sweeps (deferrals 2, 1, 0). The
  attempts stay 3-6, 2-3, 0-2. `path_max = 0`, `waits_neighbor = 0`, `path_splits = 0`.
- (d3) `SkewedBox(true)`: sweep 1 marks 2-3 and 0-2. 2-3 returns `Border` before `Place`. 0-2 is split (the ring
  of 0-2 is not a border ring) and removes both asking faces. One marked sweep, one `Place` at 0-2, one `Border`
  refusal at 2-3, no face left.
- (f1) the fan: sweep 1 marks pa and pc; equal lengths put pa first by id; the cell that they share defers pc; pc
  is split in sweep 2. Two attempts, one split in each sweep, zero steps and zero waits.
- (f3) the floor on the walk needs a first edge that is NOT in the table: the cell p = (0, 0, 0), a = (1, 0, 0),
  b = (0.5, 0.2, 0), c = (0, 0, 0.02); face pab has only a centroid error, face pac a pooled error on pc. The
  first edge is the longest edge pa (not in the table); the next chosen edge pc is below the floor (0.028 in this
  fixture), so the walk is locked before the stop: `NeighborLocked`, `Short`, `path_max = 1`, no `Place`.
Run every other existing fixture again. Derive the option-on cylinder (a) again (the residual reasons and the
comparison of the split counts). In (b) and (g) keep the pins "diagonals first, across edges second" if you derive
the same marks and rings. Where a pinned value changes, write the old and the new value in the report.
3.2 New fixtures (hidden tag `[.][NativeFP2D]`; fake sheet geometry, so that a rim is not a feature):
(k) The strip: one sector of `Cylinder` with four and with eight layers of equal height (strips of 8 and 16
    faces), with `DistanceSampleGeometry`; each shared edge of the strip gets the same rising fake error on BOTH
    original faces at it; every other sample, also on children, is zero. Assert for the rule of build656 (in a test comment, or with a test switch if one exists): only the last
    edge is marked in sweep 1. For the new rule: all shared edges are marked in sweep 1; the longer diagonals of
    the panels, whose rings do not meet, are split first and the other marks are deferred; expected by the review:
    these splits remove all sampled original faces in ONE marked sweep, for both lengths. Verify and pin.
(l) The walk that stays: the centroid-only route of `SkewedBox(false)`, 0-2 -> 2-3 -> 3-6 (no edge in the table):
    as on build656. The same with Err for 2-3 given by face (2, 3, 6): the walk stops at 2-3 after one step.
(m) The feature exception: the route of (l) with the sheet fake changed so that 2-3 is an above-limit FEATURE
    edge; face (2, 3, 6) has pooled errors on 2-3 and a stronger one on 3-6; the centroid request stays on face
    (0, 1, 2). Assert that sweep 1 has exactly ONE candidate and one split, at 3-6, with no deferral (a wrong stop
    at the feature edge would mark 2-3 too and defer it behind 3-6).
3.3 The probe N1Invariant (section 5 of the order): an accepted split of step D passes when it is a NONFEATURE
edge in the independently rebuilt Err of its sweep, OR each physical parent chooses it. Keep the sample capture,
the sweep reset, the count of acceptances and the classes error / longest. `checked == splits_distance`,
violations 0, no throw of the guard. Use the same probe for fixture (a).

## 4. What of the order no longer holds

- Section 1 of the order, its three statements about which edge can cut a face ("no other edge is taken when the
  chosen edge of a face is locked", "a face is cut only through its own chosen edge", "a face whose chosen edge
  is locked is left"), and in 3.4 the sentences on the end of the path and on a locked choice: replaced by section
  2 here. A locked choice stops that face from asking for another edge. It does not protect the face against a
  nonfeature error mark of a neighbour.
- The comment at the path (3.4): replaced by section 2.
- Section 4 of the order: the expected results of (d2), (d3), (f1), (f3): replaced by 3.1. The sweep numbers of
  the cylinder cases were model predictions: derive them.
- Section 5 of the order: the condition of the probe: replaced by 3.3.
What stays: the feature-parent rule, the strict rise of the key at each step, the limits of the item, 3.1 to 3.3,
3.5, 3.6, the lab with ALL its gates (also `sweeps_distance` below 40, the altitude condition, the metric-edge
gate, `star_max`), the alarm, the regional report, the run S, the labels, the failed-gate rule.

## 5. The order of the work

0. One R4 source check of this continuation with what it inherits. Keep the evidence of build656.
1. The change of section 2 and the fixtures of section 3. Build `SU2_CFD` and `UnitTests/test_driver` without a new
   warning. Run: all FP2-D fixtures; the option-off coverage (i); all 18 FP1 cohorts; G1 identical
   (375 / 114 / 228). Seal the receipts. Commit and push ("FP2-D continuation 1: fixtures passed; default path not
   yet proven").
2. The lab of section 5 of the order on the new build, complete (N1, N6, N6P two times each; N1Invariant with 3.3;
   the alarm). Report beside the reference AND beside the failed trial of build656 (N1: the splits for each sweep,
   `sweeps_distance` 40, `waits_neighbor` 55,131, `path_max` 184, 3,926 splits, `left_limit` 1,176, final faces
   50,222, final faces above the tolerance 1,105, smallest altitude 7.0005e-5, smallest face angle 0.2235 deg,
   shortest edge 9.2193e-5, smallest metric edge 0.0148): the same values of the new build, for each case; the
   split refusals (`Near` included) and the faces left by reason (`Refused`, `Short`, `NeighborLocked`, `Limit`);
   and the regional report (the size along the span at the nose shows whether the anisotropy is kept). Then the label of the order ("FP2-D: lab passed;
   default path not yet proven", or its alarm / FAILED label).
3. The run S and the close: items 3 and 4 of section 6 of the order, not changed ("FP2-D: S passed; default path
   not yet proven"; the report also has the history of the failed lab of build656). Then stop and wait, as the
   order says, unless a later user order says what comes next.
The failed-gate rule of the order holds for each step (keep the code; one read-only diagnosis; one correction and
one repeat only for a wiring error against this text; else the FAILED label, stop and wait).

## 6. Not in this continuation

A change of the key, of the table Err or of the choice of the asking faces. A stop for a FEATURE edge only because it is in the table (a feature edge that each parent chooses is still an end).
A change of step L, of step C, of rule 2, of the floors or of the 40-sweep limit. A new shape limit for the faces
(the smallest face angle and the smallest altitude of the lab are reports and the altitude condition of the
order).
