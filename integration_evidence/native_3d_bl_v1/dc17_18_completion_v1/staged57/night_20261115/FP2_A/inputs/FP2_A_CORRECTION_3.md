# FP2-A correction 3: step D splits only the longest edge of a face (no fallback), with a length floor

Author: reviewer, 2026-11-21 (checked against the source). For the main agent. The user sends it; it is a user order.
It changes ONE rule of `FAST_PATH_STAGE2_A.md` ("the text"), section 6, step D, item 2, and adds print rows. All other
parts of the text and of corrections 1 and 2 stay. Copy this file with its SHA256 into
`R/night_20261115/FP2_A/inputs/`. R12 holds: do not stop a job that runs; apply this at your next boundary.

## Why

Your rank check was correct and it found a fault of the text. In K1 the surface steps of pass 1 raised rank 1 to
511,584 cells with one point in 98,384 cells, and rank 0 to 244,759 cells with one point in 65,654 cells (9 and 11
points have more than 1,000 cells). 65,654 is 2^16 + 118: a fan of faces around one point that doubles in each sweep of
step D. The volume work then needs minutes for one sweep (`Star` costs the square of the count of cells at a point).
The cause is rule 2 of step D in the text: "mark its longest edge that is not a feature edge and not in the refused
set". When the long edges of a face cannot be split (refused for any reason), this rule falls back to the next edge,
down to the shortest one. The face is then cut again and again along its short side: each child keeps a long edge with
the same distance to the model, stays above the limit, and is cut again. This is the mechanism of the folded root faces
of FP2-0; the text closed it only for feature edges. Which refusal starts it with 2 ranks is not known yet (the row of
the pass was never printed); the new rule does not depend on the reason.

## 1. The change (production; step D of section 6 of the text)

Item 2 of a sweep of step D becomes:
2. For each face whose largest sample is above `Limit()`, the first rule that fits:
   - one of its edges is a feature edge with a chord above `Limit()` (marked in 1, or in one of the two sets): nothing
     more for this face in this sweep (reasons as in the text);
   - else take E = the LONGEST edge of the face (length in space; equal lengths: the smaller id, then the larger id),
     of any kind (a feature edge too: `SplitSurface` places its point by 5.2). Then:
     E is in the refused set: mark nothing (reason `refused`);
     E is in the border set: mark nothing (reason `left_border`);
     the length in space of E is below `2 * Limit()`: mark nothing (reason `short`);
     else mark E.
   No other edge of the face is marked. There is no fallback to a shorter edge.
The rule "feature_dominated" of the text is removed (print the field with 0, or remove the field).
Item 1 (feature chords) gets the same floor: a feature edge shorter than `2 * Limit()` is not marked.
Items 1 and 3, the two sets, the order, the stamp and deferral rule, the stop rule and the limit of 40 sweeps stay.
Step L does not change.
Write the reasons as comments:
- Bisection of the longest edge of the face that asks for the split keeps its smallest angle bounded and never cuts
  its short side, so it cannot make a fan; a rule that cuts a shorter edge can. A face whose longest edge cannot be
  split is left for pass 2 or for the engine.
- The floor: on a surface with smallest principal radius R, a face with longest edge l is at most l^2 / (6 R) away from
  it (a chord: at most l^2 / (4 R)). With l below 2 * Limit() a distance above Limit() needs R below 2 * Limit() / 3: a
  detail smaller than the tolerance, which a split cannot resolve.

## 2. Print rows (print-only, R11; no decision reads them)

1. The row `NATIVE3D_LOCAL_SURFACE` gets `left_short=` (faces left by the floor) and, after the surface steps of the
   pass: `star_max=` (the largest count of live cells that contain one point), `star_gt100=`, `star_gt1000=`.
2. One row for each sweep of step L and of step D through the present sink (`stats.sink`, as the volume sweeps):
   kind `surface_length` or `surface_distance`, sweep, candidates (step L) or marked edges (step D), accepted splits,
   refusals by reason, edges put in the border set, deferred, live cells, seconds. If `SweepRow` cannot hold these
   without a change of a type that makes `Mesh::BytesPerCell`: use the fields that fit and write what is left out.

## 3. What to repeat, and the order of the work

1. Fixtures 2, 3 and 4 of the text and LAB-S (one rank), with the pass conditions of the text and the two alarms of
   correction 1 section 2 item 2. Write the new row beside the rows of build621 and build624, with `left_refused`,
   `left_short` and the count of faces with `Classify` above tol. The pass condition of LAB-S for the proof changes
   (the new rule can leave a face that the old rule cut; in production the engine does these): the proof accepts each
   entity, OR each face with `Classify` above tol is a face that step D left with a reason; write the counts by
   reason. A face above tol that step D did not leave with a reason: LAB-S fails (R5).
2. The rank check of correction 2: K1 (2 ranks) and K3 (6 ranks), surface on (K2 is not necessary again), with its
   prints and its rule of section 2 (item 1: continue; item 2: stop before 11.3). One more alarm for both runs: a
   `star_max` above 1,000 after the surface steps on a rank: treat it as "did not end" (stop before 11.3). A second
   alarm for each rank: `faces_out` above 2 times its share of one rank (`faces_in` of that rank times
   84,088 / 48,954), or `sweeps_distance` = 40: also "did not end". For an alarm write in the report: the sweep rows of
   step D, its refusal counts, `samples_unconverged`, and for the 5 largest stars the point (id, coordinates, the
   flags `surface` and `border`) and the refusal reasons of the edges at that point.
3. Build without a new warning; all fixtures of FP1; G1 identical. Commit and push ("FP2-A correction 3").
4. The gates of 11.2 and 11.3 use a binary that has this correction. ONE exception: the G5 run of 11.2 with both
   options NO that ended, or was running when you read this, on a binary without it counts when all of these hold:
   (1) its `run.cfg` does not set `ADAP_NATIVE_LOCAL_SURFACE= YES` and its output has no row `NATIVE3D_LOCAL_SURFACE`
   with `applicable=1` on a rank; (2) `git diff --stat` between that build and the new build touches only
   `CNativeLocalCore3DSurface.cpp`, `CNativeLocalSurfaceGeometry3D.cpp`, the surface declarations of their headers and
   test files, and no function that runs with the surface option NO (not `PrintSweepFields`, not `RunPass` outside its
   surface branch). Write both builds, this diff and the reasons for (1) and (2) in the report. The short gates of 11.2
   (G1, G2, GQ, G3, G4, the safeguards), the run CORE and the runs S1 and S2 all use the new binary; no exception for
   them (the time rule of 11.3 compares runs of one binary).
5. If a G5 run of 11.2 has not started yet: do this correction first.

## 4. Not in this order

A change of step L, of the volume work, of `Star`, of the test `Blocked`, of a limit of the core. Any other change of
the text.
