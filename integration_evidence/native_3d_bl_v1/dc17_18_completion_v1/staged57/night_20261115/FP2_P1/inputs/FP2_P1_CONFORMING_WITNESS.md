# FP2-P1: conforming joined witness for the geometry proof, behind a process switch (default off)

Author: reviewer, 2026-11-25 (design reviewed by Codex in two passes; source facts by Codex). For the main
agent. The user sends it; it is a user order. Copy this file with its SHA256 into
`R/night_20261115/FP2_P1/inputs/`.
Code base: `3228ca0e1e` ("FP2-M3: measurement done; default path not yet proven"). Production = build664 plus
the print-only trace rows of FP2-M3.
Short names: AR = `Common/src/adaptation/CNativeAffineReference.cpp` (lines of `3228ca0e1e`); M2, M3 = the
orders FP2-M2 and FP2-M3 (your copies in `R/night_20261115/FP2_M2/inputs/`, `.../FP2_M3/inputs/`); "the lab" =
the lab of FP2-C3 continuation 1 with its campaign, runner, comparator and evaluator; "the run S" = the run S
of FP2-C3 continuation 1 with its launcher and its isolated restart (sealed result of build664:
`R/night_20261115/FP2_C3/G5/build664/S/`). "Closure" = the change of section 2. "Switch" = the process
setting of 2.1; "off" / "on" = closure `0` / `1`.

Scope. A production change of shared proof code in ONE file (AR), behind ONE process switch. With the switch
off the program must do what build664 does: no result, no charge, no modeled byte, no key and no member
changes. With the switch on, the joined witness of the curved prepared path is closed before its
`TriangleUnion` is made. The proofs stay proofs (section 3); the work, the seconds, the bytes of a prepared
scope, and in principle a decision can change. So each result with the switch on is an OBSERVATION of this
order. It is never a reference and never an acceptance.
Not evaluated in this order (write each as "not evaluated" in the report): G2, GQ, G3 and G5 with the switch
off, and the full two-event no-BL workflow with the switch off (they belong to the default proof of the whole
fast path, which is still open: "default path not yet proven"); the 18 FP1 cohorts; a second run S; the switch
on by default. The switch on by default needs a later user order with the two-run rule.

## 1. Why

FP2-M3 (N1, tolerance 1e-4, 68,853 measured uncached step-C queries): the queries whose root face is not
proved with one prepared node hold 85.7 % of the measured result work, and the queries with a T-junction of
the chord subdivision at the inside-midpoint test hold 87.5 % of that work. `BuildPlanes` joins two chords
directly only through an edge with the same two vertex ids; a coarse chord edge (A, B) and the chain of fine
edges A - V1 - ... - B beside it have different keys. This can prevent a common footprint. The long queries
prepare about 400 nodes on average, and each of them ends within the tolerance. The measurements do not
isolate the reason of the bound for each query. The same query serves the engine.
Repair: replace a coarse chord by a fan of sub-chords through the points of the fine chain. Then the edge keys
match and chords with the same nonzero (axis, sign) on both sides can join.

| Assumption | State |
|---|---|
| A sub-chord with the error rule of 2.4 keeps each property that the proofs need of a witness chord | proved in section 3 (reviewed by Codex). Check the source statements of section 3 in your R4 source check |
| With the switch off the program is bit-identical to build664 | from the form of the change. Tested by step 1 |
| The share of the completed miss / bypass forwards that prove the root with one prepared node rises | NOT KNOWN. Measured with the grouping of M2 in step 2.3. No gate |
| The closure changes no decision (the meshes with the switch on are those of build664) | NOT KNOWN. Reported in steps 2 and 3. No gate |
| The bounded greedy search finds and commits geometric chains | NOT KNOWN. Counted (2.6). These counters do not say that a chain is a real seam, and they do not give the root-pass share of the T-junction queries of FP2-M3 |
| The result work of step C falls from 477 M units to about 135 M | a counterfactual of the reviewer, not a promise. Measured in step 2 |

## 2. The change

2.1 The switch. An environment variable `SU2_NATIVE_CONFORMING_WITNESS`; the value `1` is "on", each other
value and no value is "off". Read it ONE time in the process with a function-local static, in the style of
`AtlasOps::Trace()` (AR:148-152). Call that function at the entry of `BoundPhysicalSurface` (AR:2238), before
the memo and before each bypass, and at the entry of `QuerySurfaceUnion` (AR:2393), before the preparation.
Only the place of 2.2 uses the setting to change behaviour. No config option, no member of the authority, no
bit in a key. When the setting is on, print ONE line to stderr at that first read:
`NATIVE_SURFACE_CONFORMING_WITNESS enabled=1`. With the setting off no line is printed. (This line and the row
of 2.6 are the written exception to R11 for this item: they follow the process switch and the trace switch,
not `options.timings`.)

2.2 The place. Only the scope miss of the curved prepared path: the joined list is
`prepared_model->triangles`; close it after the existing `NATIVE_SURFACE_WITNESS` row (AR:2601) and before the
`TriangleUnion` constructor (AR:2602). Not changed: the facet leaf lists and their caches; the local model path
without the prepared pool; the physical-face targets of the model-to-mesh direction; `selected`, the scope
key, `selected_pieces`, `support_pieces`, `forest.complete_witness` (it stays
`selected.size() == model_pieces`) and `forest.maximum_error` (the largest error of the ORIGINAL facet leaves).

2.3 The closure (a bounded greedy pass). Input: the joined list L of chords; a chord has three stored points
and a certified error. Write it as ONE pure function of (L, resolution, budget) that returns the closed list,
for each chord of the closed list the index of its original chord in L, and the counters of 2.6 except
`left_closable`. It reads no other state. Small helper functions (R9).
State of the pass:
- Vertex ids: by the coordinate comparison of `BuildPlanes` (AR:633-647), given at the first occurrence in
  ascending order of the input chords and of their local corners 0, 1, 2. They are frozen: the closure makes
  no new point.
- For each chord of the working list: its three points; its ORIGINAL error (the error of the input chord that
  it comes from); for each of its three point instances an offset d (0 for the three points of an input chord).
- For each edge key (sorted pair of ids): `count` = the number of different chords of the working list that
  have the key, and the chord that has it when the count is 1 (the "owner"). Both are kept current after each
  commit. An edge is "open" when its count is 1.
- lim = the greatest binary64 number that is not above the exact resolution / 8.
The pass: one loop over the working list by ascending index (the chords that a commit appends are in it); for
the chord X at the index inspect its local edges k = 0, 1, 2 in this order; after a commit, inspect the chord
that now stands at the same index again from k = 0. The pass ends at the end of the list. It does not go back
to an earlier index.
Inspecting the edge (A, B) = (point k, point k+1 mod 3) of X, with C the third point:
1. The key (A, B) must be open. X must have a NONZERO sign by the rule of item 3. Else: next edge.
2. Chain. Compute in binary64, round to nearest, with no contraction to a fused operation: the coordinate
   differences u = B - A; the denominator q = (u0 u0 + u1 u1) + u2 u2. If an intermediate is not finite or q is
   not positive: next edge. current = A, t_current = 0.
   Iteration: for each OPEN key (current, W) whose owner is not X, with W not A and not C (an "adjacency"):
   if W is B, it is a candidate with t = 1. Else compute w = W - A, the numerator
   p = (w0 u0 + w1 u1) + w2 u2 and t = p / q in the same arithmetic; keep t fixed. W is a candidate only when
   each intermediate is finite, t_current < t < 1, and delta(W) is finite and not above lim. delta(W) is an
   upper bound of |W - (A + t (B - A))|: enclose A + t (B - A) at the stored t with `Exact` and the interval
   `Multiply` and `Add` for each component, take `Difference` to `Exact(W)` and `NormBound`; check that each
   interval end is finite before you use an enclosure. These tests come BEFORE the choice.
   Choose the candidate with the smallest (t, vertex id). No candidate: the chain fails as `dead_end` (no
   back-tracking). The choice is B: the chain is complete. Else current = the choice, t_current = its t, and
   iterate. A chain has at most 8 edges: if the eighth edge does not reach B, the chain fails as `long`.
   (A chain of one edge cannot occur: the key (A, B) has only X.)
3. Fan. With the chain A = V_0, V_1, ..., V_n = B (n >= 2), the proposed children are (V_i, V_i+1, C) for
   i = 0 .. n-1, with the orientation of X. Checks in THIS order; the first failure rejects the split and X
   stays unchanged:
   - `fan_key`: for each inner V_i the key (V_i, C) has count 0 in the working list;
   - `not_finite`: each offset and each child error of 2.4 is finite;
   - `orientation`: for each child, the lower end of `DotBox` of its normal box (`CrossBox` of its two edge
     differences, as AR:637-642) with the normal box of X is above zero;
   - `projection`: each child has the same axis and the same nonzero sign as X, by the rule of `BuildPlanes`
     (axis = the largest |lo + hi| of the normal box, the first axis at a tie; sign = +1 or -1 only when the
     interval of that component is strictly on one side of zero, else 0).
4. Commit. Child 0 takes the place of X in the working list; the children 1 .. n-1 are appended at the end in
   this order. Update the keys: (A, B) goes away; each chain key goes from count 1 to 2; each key (V_i, C) of
   an inner V_i goes from 0 to 2; the keys (C, A) and (B, C) keep their counts, and where their count is 1
   their owner becomes child 0 and child n-1.

2.4 Offsets and errors (certificates). Each sum below is made with `Add` on singleton intervals, and its upper
end is converted UPWARD to binary64.
- Offset of an inner chain vertex V_i as a point instance of the children: d(V_i) = delta(V_i) + max(d(A),
  d(B)), with d(A), d(B) the offsets that A and B have as point instances of X. The instances of A, B and C
  keep in the children the offsets that they have in X.
- Error of a child = the ORIGINAL error of X + the largest offset of its three point instances.
- A child stores this error as its chord error: the index (the boxes grown by the error), the single-chord
  upper, the lift and the distance of the `TriangleUnion` use it as they use the error of each chord.

2.5 Work, storage, deadline. Charge ONE `op.Work()` before: each input chord that is put into the maps; each
local edge that the pass inspects; each chain iteration that is entered; inside an iteration, each key that
meets the structural definition of an adjacency in item 2 (an open key (current, W), owner not X, W not A and
not C), before its foot and its delta are tested. For a fan: first make all `fan_key` checks (no charge).
If they pass, visit the children in ascending i and charge one `op.Work()` immediately before the offsets and
the error of that child are checked (`not_finite`); stop at the first failure. If all children pass, check
`orientation` in ascending i, then `projection` in ascending i, with no more charge. The deadline check is the
periodic one of `ChargeWork`; add no other clock read.
Storage: the capacity of the joined list is already in `caller_bytes` (AR:2600); `op.Storage` checks an
admission against the live total and does not reserve. So each admission of the closure, made BEFORE its
allocation, names all closure scratch that is alive at that moment (maps, owners, offsets, origin indices) and
the capacity of the new list while the old list is still alive. Free the scratch before the `TriangleUnion` is
made, or keep it charged while it lives. Before the `TriangleUnion` is made, the charge of the list is that of
the FINAL capacity (replace the charge of the original capacity). Keep the restore of `caller_bytes` and the
present behaviour when the retention of a built scope is refused; `PreparedFacet::Bytes()` reads the final
list by itself. A refused admission is a
failure of the query as each other refused admission of this build: there is NO fall-back to the list that is
not closed. With the switch off none of this code runs.

2.6 Row (print-only). When the switch AND the trace switch are on, one row after each closure and before the
`TriangleUnion` is made:
`NATIVE_SURFACE_WITNESS_CLOSED chords_in= chords_out= splits= children= chain2= chain3= chain4= chain5_8=
reject_dead_end= reject_long= reject_fan_key= reject_not_finite= reject_orientation= reject_projection=
left_closable= origin=<for each chord of the closed list the index of its original chord, with commas>`.
`splits` = the committed fans; `children` = the children of all committed fans, child 0 included; `chainN` =
the committed chains with N edges. One reject is counted for an inspected edge at its FIRST failed check, in
the order of 2.3; a chain failure (`dead_end`, `long`) is counted only when the walk had chosen at least one
candidate before it failed; an edge that item 1 skips, or whose walk finds no first candidate, counts nothing.
`left_closable`: after the closure, a scan of the final state that counts the open keys for which items 1 and
2 of 2.3 still give a complete chain. The scan is outside the pure function, runs only for this row, reads
only, and never calls `Work`, `Storage` or a time check of the live budget. The whole trace-only part (the
scan, the preparation and the print of the row) must not throw into the query: use the fault isolation
`TraceSurface` of FP2-M3 for it; a failed scan prints `left_closable=-1` with the other fields kept; a row
that cannot be made prints `NATIVE_SURFACE_TRACE_INCOMPLETE site=closed`. A trace-only fault never changes the
closed list, the M2 tables or the inherited result projection. The rows of M3 stay as they are; with the switch on
the `PLANE_*` rows of M3 name the chords of the CLOSED list and the `WITNESS` row names the original selection.

## 3. Why the proofs stay valid (for your source check; reviewed by Codex)

What the code needs of a chord of the joined target: (U) for the single-chord upper and for the lift: each
point of the flat chord is within the chord's own error of the model (AR:198-222, 434-450, 468-625, 723-726);
(L) only when the witness is complete, for the sample-distance lower and the index: each model point of the
pieces of the witness is within the error of at least one chord, of that chord's own error (AR:378-433,
723-756). The footprint coverage and the lift are computed from the stored points of the chords as they are;
`FinitePlaneReference` still validates each footprint (AR:658-715, 3569-3595), and a group that is `invalid`
or `trimmed` keeps its chords in the ordinary distance path as today. The checks `orientation` and
`projection` give each new child a valid orientation.
Virtual subdivision. An input chord T with error e has: the model piece of T and the flat T differ by at most e
at each parameter (AR:369-376). Each point INSTANCE R of a chord of the working list has a virtual point P in
T: P = R for the three instances of the input chord; an instance that a child takes over from its parent
keeps the virtual point of the parent; an inserted chain vertex with the fixed parameter t on the parent edge
with the instances (R_a, R_b) gets P = P_a + t (P_b - P_a) exactly. Then
|R - P| <= delta + (1 - t) d_a + t d_b <= delta + max(d_a, d_b), which 2.4 stores rounded up. The parameters of
a chain are strictly increasing in (0, 1), so the virtual children of a fan partition the virtual parent, and
by induction the virtual descendants partition T.
(U): a point of a child and the point of its virtual child with the same weights differ by at most the
largest offset; that point is in T, so within e of the model. (L): a model point is within e of a point p of
T; p is in one virtual child; the point of the real child with the same weights is within the largest offset
of p. So the cover holds child by child with the child's own error, and each model point of the piece is in
the grown box of at least one child. `complete_witness` stays the test on the original selection, and the
approximation floor (AR:2678-2685) stays the largest ORIGINAL leaf error (AR:2539). The proof uses no leaf
parameter and no facet: it holds for each chain that passes the tests of 2.3, also for one that is only near
the edge.

## 4. Fixtures (hidden test cases; G1 does not change)

Declare the pure function of 2.3 in a private header under `Common/src/adaptation/` in a named internal
namespace, so that the existing unit-test translation unit can call it (no installed public declaration, no
member of a production object). New hidden cases in the existing unit-test files:
(a) direct cases of the pure function (trace off), each with the asserted closed list, origin indices, errors
and counters: a T-junction of depth 1 between two "facets" and one inside a facet; a fully quartered edge (4
chain edges; the keys (A, M), (M, B) do not exist); a mixed chain (A - M - Q - B); chains on two edges of one
chord; a far false neighbor (a vertex joined to A and B by open keys, farther than lim: no split); a near
false neighbor (within lim, not on the edge: split, with the offset and the error asserted); a dead end with a
later near vertex (no split: tests before the choice, no back-tracking); a chain of 8 edges (split) and one
that needs 9 (no split); an inner vertex whose key to C exists (no split); a child that would be inverted and
one with zero area (no split); a parent with sign 0 (no split); a key with three chords (not open: no split);
an input without T-junctions (the output list is bit-identical to the input);
(b) the error rule on (a) cases with a known analytic surface (a sphere patch is enough): for a grid of points
of each child, the distance to the surface is at most that child's stored error; for a grid of surface points
p of the piece, AT LEAST ONE child i has distance(p, child i) <= error of child i (the nearest child need not
be that one);
(c) through the public query, in separate hidden test-driver processes, one for each switch setting, with the
trace on for the assertions on nodes and footprints. The test geometry must have a facet with deeper leaves
than its neighbor under the physical face (the strip of FP2-M1 has only depth-zero chords: change the test
geometry or make a new small reference; R3):
- improvement: with the switch on the query proves the face with ONE prepared node; with the switch off it
  prepares more than one node; both answers are "within";
- lower path, one case with a complete witness and one with a cropped witness: the physical triangle is such
  that the upper attempt enters the sample distance; show it with `full=` of the `NATIVE_SURFACE_MODEL` row
  and a positive `distance_work` of a physical `DOMAIN` row; assert the classification and the expected
  relation of the bounds for both switch settings;
- eviction: with `BoundSurfaceUnion` (so that no face-memo hit hides it) show that a closed scope is retained,
  evicted, built again on a later miss, and gives the same bound bits (the pattern of the existing cache test,
  `UnitTests/Common/adaptation/CNativeAffineReference_tests.cpp:1202-1244`).

## 5. The work, the gates and the labels

A new build number and receipt folders under `R/night_20261115/FP2_P1/`. Build `SU2_CFD` and
`UnitTests/test_driver`, no new warning; freeze and pin this binary pair. One owned heavy job; the host
preflight of R1 before each launch.
Runners (R3: they are yours to make work).
- Source-diff check: permitted are the authorized changes of AR, the one private test declaration, and the
  hidden test additions in existing unit-test files. All of them are pinned.
- The two switches. Each launcher sets `SU2_NATIVE_CONFORMING_WITNESS` and `SU2_NATIVE_GEOMETRY_WORK_TRACE`
  explicitly, in the environment of the actual child and AFTER each cleanup that a launcher does (the G1
  candidate launcher and the shared-engine launcher remove `SU2_NATIVE_*` variables: an assignment on the outer
  command is not enough), exports them to each MPI rank (lab, shared checks, run S, isolated restart), and
  records the values that the child got. The trace is `0` for the shared checks, the run S and the isolated
  restart, and for the direct cases 4a and 4b (which run with closure `0`: they call the pure function).
  With closure `1`: require the line of 2.1 one time for each process that enters one of the two query
  entries; record the ranks and processes that enter none apart (for example
  the 2D checks). With closure `0`: require that no such line is there.
- Checkpoint. The lab runner, the comparator and the launcher of the run S of FP2-C3 ask for a
  `c3_acceptance.json` with the 18 FP1 cohorts of the current build. In this item the 18 FP1 cohorts are not
  evaluated. Adapt these three helpers under `FP2_P1/`:
  - the trial of step 1.3 is launched after a check of the frozen binary pair, the sources, the runtime, the
    inputs and the results of steps 1.1 and 1.2 only (not of the whole step 1);
  - when step 1 is finished, seal a P1 checkpoint of the current binary pair with the status and the evidence
    of each part of step 1 (G1, the inherited fixtures, the N1 trial, the five shared checks, the new hidden
    cases with their settings). Step 2 may start from a checkpoint whose step label is "passed" or "OPEN",
    when the launch pins verify;
  - the comparator of step 2 and the launcher of the run S verify that checkpoint; the launcher of the run S
    also verifies the six qualified trials of step 2 and their three equal pair comparisons;
  - use different receipt roots for the N1 trial of step 1.3, for the campaign of step 2.1 and for ANATOMY,
    with the attempts apart inside each root.
  Verify the references of build664 with their own original checkpoints and validators. Keep the inherited
  validators, the source and runtime pins, the workflow and the isolated restart.
- Campaign: a trial that ends OPEN does not stop the trials and checks that do not depend on it (the
  inherited campaign driver returns at the first OPEN trial: change that in your adapter). Compare a pair
  only when both of its trials qualified.
- Watchdogs of the eight lab trials of this order: the wrapper limit is 3,261 s for N1, 2,575 s for N6P and
  4,522 s for N6 (three times the sealed wall time of trial 1 of build664, rounded up). One retry with the
  same limit after a wrapper-only timeout, when all owned processes are terminal. Change the checks of the
  parent, of the owned child and of the receipts of the C3 lab runner consistently to these values. A failure
  that was seen wins over the watchdog; the attempt rule and the partial-evidence rule of M2 section 3 hold.
  The run S and its isolated restart keep their inherited wrapper and retry limits. No in-process deadline
  and no result gate changes.
- Shared checks ("the five shared checks"): first collapse; exact C492; `merit481`; `saved_current`; native 2D
  BL / no-BL at 1, 2 and 4 ranks; each with the frozen binaries of this order and its unchanged inputs,
  validators and limits. (The fourth entry of `R/shared_engine_regression_gates.json`, the full no-BL
  workflow, is not run with the switch off; with the switch on the run S of step 3 is that workflow.)

Step 0. One R4 source check of this text (sections 2 and 3 included).

Step 1, the default path (closure `0`) and the new cases:
1. G1 (closure `0`, trace `0`): identical to the reference (375 / 114 / 228). The new cases are hidden.
2. The focused fixtures of FP2-C3 continuation 1 (9 serial, 3 two-rank; closure `0`, trace `0`) with the Catch
   counts of build668. The curved fixture of FP2-M1 (closure `0`, trace `1`) with both checkers of FP2-M3.
3. N1, one lab trial (closure `0`, trace `0`; option T on with `1E-4`; M1 and the exporters of M2 / M3 off):
   `repeat_differences` against the sealed trial 1 of build664: equal, the compared `modeled_peak` fields
   included.
4. The five shared checks with closure `0`, each with its own acceptance.
5. The new hidden cases of section 4 (4a and 4b with trace `0`; 4c as section 4 says).
Label: "FP2-P1: fixtures passed; default path not yet proven", or "FP2-P1: fixtures FAILED; default path not
yet proven", or "FP2-P1: fixtures OPEN; default path not yet proven".

Step 2, closure `1` (lab, observation):
1. The lab campaign: N6P, N1, N6, two trials each (trace `0`), with the inherited gates of each trial and the
   three exact pair comparisons (trial 1 against trial 2 of the same case). The two N1 trials and ANATOMY use
   the same pinned inputs and the same lab settings: `ADAP_HAUSD= 1E-4`, option T on, the lab option of FP2-M2
   on (`SU2_NATIVE_FP2_M2=1`: the stage rows, and the marks when the trace is on), the M1 observer off, both
   CSV exporters off.
2. Report, not a gate: `repeat_differences` of trial 1 of each case against the sealed trial 1 of build664
   (equal, or the first row, field, rank and pass that differ, and the counts of faces, cells and removals of
   both); elapsed seconds, pass seconds, step-C seconds and the Bound clock beside build664.
3. ANATOMY: N1, closure `1`, trace `1`, with the marks, the stream and the aggregator of M2 2.3 and the stage
   rows of M2 2.2 (the collector and the analysis of M3 are NOT used). Extend the M2 aggregator: over the
   completed forwards of class `miss` or `bypass`, in its grouping by length bin and `direct_eligible`, sum
   each counter except `left_closable` of each valid `NATIVE_SURFACE_WITNESS_CLOSED` row (also of a row with
   `left_closable=-1`); sum `left_closable` only where it is not negative; report the counts of the rows with
   a successful and with a failed scan, and of the forwards with an incomplete closure row. Tables beside
   those of the ANATOMY trial of FP2-M2: forwards by class; the share with `root_enough`; prepared nodes;
   result work and its parts; the mean of `groups` for each emitted `PLANES` row with its denominator; the
   stage shares; the closure counters.
   Gates of ANATOMY: its inherited gates pass, and the C3 `repeat_differences` against the selected qualified
   trial 1 of N1 of step 2.1 (same binary pair, closure `1`, trace `0`) is EMPTY. A confirmed difference with
   a complete projection is a result-gate failure with closure `1`. With a projection that is not complete
   the comparison is "not evaluated". Report the completion of the trace measurement apart.
4. The five shared checks with closure `1`. A difference of a historical decision-identity value (for example
   `accepted_by_rank`) is a "missed comparison": report it and mark it OPEN for the user; it stops nothing and
   needs no revert. A failure of execution, validity, geometry, quality, work, resource or in-process
   deadline is FAILED. Keep the raw result and the first failing field of each check.
Label, by the common label rule: "FP2-P1: lab passed; default path not yet proven" only when EACH required
part of step 2 passed; "FP2-P1: lab OPEN; default path not yet proven" when no part failed and a part is OPEN
or not evaluated (a missed comparison of item 4 makes the step OPEN); "FP2-P1: lab FAILED; default path not
yet proven" when a required part failed.

Step 3, closure `1`, trace `0` (one run S with its isolated restart, observation). Start it only when all
six trials of step 2.1 of the current binary pair qualified, their three pair comparisons are equal, and no
required part of an earlier step is FAILED. An OPEN anatomy or an OPEN missed comparison of step 2.4 alone
does not prevent the run S. If the run S cannot start: write its parts as "not evaluated" and use the label
"FP2-P1: S OPEN; default path not yet proven". Report beside the sealed run S of
build664: the native maxima of both events; for each rank and event the sum of `exclusive_seconds` of the
`NATIVE3D_TIMER root=remesh boundary=final` rows by operation (`geometry_bound`; `communication` +
`join_wait`; all others); proof faces, hand-over faces, cells, job peak; the six payload hashes beside those
of build664 (equal or not: a report).
Label: "FP2-P1: S passed; default path not yet proven", or "FP2-P1: S FAILED; default path not yet proven", or
"FP2-P1: S OPEN; default path not yet proven".

Failures and labels.
- Use the switch setting of the check that failed.
- A confirmed failure of a RESULT gate: R5 (keep the first difference; one diagnosis of at most 60 minutes; a
  confirmed transcription or wiring fault against this text: one correction and one repeat of that check). At
  the diagnosis limit without a confirmed wiring fault the check is FAILED.
- Closure `0`: a confirmed production result failure is reverted in one commit (write the hash; the step is
  FAILED) in each of these cases: the cause is in this text; the diagnosis ends without a confirmed wiring
  fault; the permitted repeat fails.
- Closure `1`, for this item only: a production result failure that stays keeps the commit (the default path
  is not damaged), is recorded as a FAILED observation under R8, and stops the later stages that depend on
  it.
- A defect of a fixture, a launcher, a collector or a report helper (test code): R3 / R6. If a required
  measurement is still wrong or missing after that work, it is FAILED; that needs no production revert.
- A second wrapper-only timeout: OPEN, unless a failed gate was seen. A part that cannot run because a part
  before it is missing is "not evaluated". Finish each check that does not depend on a failed one.
- A production wiring correction makes a new frozen binary pair: run the checks again that it can affect.
  Never mix receipts of two binary pairs, and never label a preserved failure as accepted.
- Common label rule of the three steps: FAILED if a required part failed; else OPEN if a required part is
  OPEN or not evaluated; else passed. After "passed" or "OPEN" go on with the next step when its condition
  holds; after "FAILED" of step 1 or step 2 do not run the later steps (write their parts as "not
  evaluated").
After the last label: update the progress file, `HANDOFF_Codex.md`, `HowToProceed.md` and
`Handbook/ADAPTATION_TIMINGS.md`; write the report `R/night_20261115/FP2_P1/reports/FP2_P1_<build>.md` and
`.json` (the status of each part; the list "not evaluated"; the production diff; the tables of steps 2 and 3;
no recommendation). Then STOP and wait for the next user order.
The rules R1, R3, R5 (as read above), R6, R7, R8, R9, R10 (with the switch off) and R13 hold as in each item.

## 6. Not in this order

The switch on by default, a config option for it, a new reference, a change of a gate or of the tolerance.
A change of `BuildPlanes`, of the coverage test, of the lift, of the facet leaves, of the local model path. A
repair for the queries without a T-junction, for the scope build or for the axis / sign splits. G2, GQ, G3,
G5 and the full no-BL workflow with the switch off, the 18 FP1 cohorts, and a second run S.
