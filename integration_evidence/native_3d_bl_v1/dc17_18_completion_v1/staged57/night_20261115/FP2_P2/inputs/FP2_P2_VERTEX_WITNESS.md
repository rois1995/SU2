# FP2-P2: vertex rule for the conforming joined witness (replaces the chain walk of FP2-P1), behind the same process switch (default off)

Author: reviewer, 2026-11-25 (design reviewed by Codex; source facts by Codex). For the main agent. The user
sends it; it is a user order. Copy this file with its SHA256 into `R/night_20261115/FP2_P2/inputs/`.
Code base: `dec45f8a71` ("FP2-P1: S OPEN; default path not yet proven"): build664 plus the print-only trace
rows of FP2-M3 plus the closure of FP2-P1 behind the process switch (default off).
Short names: AR = `Common/src/adaptation/CNativeAffineReference.cpp` (lines of `dec45f8a71`); WH =
`Common/src/adaptation/CNativeConformingWitness.hpp`; P1 = the order FP2-P1 (your copy:
`R/night_20261115/FP2_P1/inputs/FP2_P1_CONFORMING_WITNESS.md`); "the lab", "the run S", "closure", "switch",
"off" / "on" as in P1. "Rule C" = the chain walk of P1 2.3 item 2 (`FindWitnessChain`, AR:473-535). "Rule P" =
the selection of 2.2 below.
"Contact" = a pair (open key (A, B) of a chord X; vertex id W) where W is not a corner of X, W is an end of at
least one open key, and W passes the foot test on A-B (0 < t < 1, delta <= lim).

Scope. FP2-P1 ended "lab FAILED": with the switch on the work rose. This order replaces the SELECTION of the
closure (rule C by rule P) and adds one fan check and one bound. All the rest of the closure of P1 stays.
A production change in AR and WH only, behind the SAME switch. With the switch off the program must do what
build664 does: no result, no charge, no modeled byte, no key and no member changes. Each result with the
switch on is an OBSERVATION of this order. It is never a reference and never an acceptance.
P1 holds where this text does not change it: P1 2.1 (switch, marker line), 2.2 (place), 2.4 (offsets and
errors), the storage and deadline rules of 2.5, the isolation rules of 2.6, section 3, the runner rules of
section 5 (read `FP2_P2` for `FP2_P1` in each path and label), and the part "Failures and labels".
Not evaluated in this order (write each as "not evaluated" in the report): G2, GQ, G3 and G5 with the switch
off, and the full two-event no-BL workflow with the switch off ("default path not yet proven"); the 18 FP1
cohorts; a second run S; the switch on by default. The switch on by default needs a later user order with
the two-run rule.

## 1. Why

FP2-P1, N1 (your report `R/night_20261115/FP2_P1/reports/FP2_P1_670.md`): the sum of `classification_work`
of the `FP2_M2 STAGE` rows went from 463,538,756 (off) to 685,848,058 (on); the `invalid` groups summed over
the emitted `PLANES` rows of ANATOMY went from 16 (68,853 rows, FP2-M2) to 11,860 (68,809 rows).
Reviewer measurement (a binary64 replay of the closure and of the footprint checks on your traces; scripts
and outputs in `reviewer_handoff/scripts/s2/p2_measure*`):
- 1,418 physical queries of the two ANATOMY samples, paired by support ids and triangle bits with equal
  input chord counts, switch off against on: where rule C splits and no group is `invalid` (242 queries) the
  work is 0.27 times the work with the switch off. Where a group is `invalid` (252 queries) the work is 1.73
  times, with 8 times the `DOMAIN` rows.
- In the replay of the P1 sample (1,190 calibrated blocks) each of the 190 replay-`invalid` groups first
  fails at a pair of border edges that matches a contact inside the group: the open edge A-B of a chord and
  a border edge that ends at the vertex W. Each of the 320 contacts inside these groups has only ONE of the
  keys (A, W), (W, B) in the witness; for 320 of 320 a recorded leaf of a support facet of the same query
  that the witness did not select has the missing half edge (the witness crop). The walk of rule C needs
  both halves, so it leaves such a contact; but it joins the two groups through other seams.
- With the switch off this does not occur: in the replay of the 70,079 records rebuilt from the N1 run (your
  provenance log of FP2-M3) no contact is inside a group (0 of 209,806 contacts).
- Rule P splits a chord at each such vertex, with or without the two halves. Replay of the 70,079 rebuilt
  records of the N1 run: rule C gives 140,100 fans and 12,246 `invalid` groups (12,229 witnesses; they hold 55 % of
  the work with the switch off); rule P gives 204,036 fans, 10 `invalid` groups, 954 `trimmed` groups (1,113
  with the switch off), 691 contacts left in 221 witnesses, 33 `long`, 312 `projection` rejects, no `tie`, no
  `chain_key`, no cap, generation 2 at most.

| Assumption | State |
|---|---|
| A fan of rule P keeps each property that the proofs need of a witness chord | proved by P1 section 3 (it does not use the walk; confirmed by Codex). Check it in your R4 source check |
| With the switch off the program is bit-identical to build664 | from the form of the change. Tested by step 1 |
| The `invalid` groups of rule C come from contacts at seams that the crop cut | MEASURED in the binary64 replay of the P1 sample. Not shown for each group of a production run |
| Rule P leaves few contacts and few `invalid` groups | MEASURED in the binary64 replay of the N1 run. NOT KNOWN in production (intervals, admissions) and for the ranks of N6 / N6P. Counted for the traced N1 in step 2 (`left_contacts`, `invalid`); not counted for N6 / N6P (trace off). No gate |
| A valid joined footprint lowers the work | MEASURED for the witnesses that rule C joins with a valid footprint. NOT KNOWN for the others. Gate D of step 2 measures it FIRST |
| The work of N1 falls by about one half | an estimate of the reviewer, not a promise |
| The closure changes no decision (the meshes with the switch on are those of build664) | NOT KNOWN. Reported in steps 2 and 3. No gate |

## 2. The change (against `dec45f8a71`)

2.1 State of the pass. As P1 2.3 (vertex ids, working list with original error and offsets, keys with their
owners, lim), and in addition:
- Live count: for each vertex id W, open(W) = the number of UNORDERED keys with an end at W that have
  exactly ONE distinct chord as owner (a key (W, W) counts one time; the two directed lookup entries of
  `WitnessTopology` are one key). Keep it current by the net change of each unordered key (the input loop and
  each commit).
- Border list B0, made ONE time after the maps are filled from the input and before the pass: the ids with
  open(W) > 0. Axis s: for each axis, extent = ONE binary64 subtraction (round to nearest) of the largest and
  the smallest coordinate of the input vertices; s = the axis of the largest extent, the first axis at a tie.
  Sort B0 by (coordinate on s, id). B0 is frozen. Between two inspections the set of ids with open(W) > 0
  never grows (a commit makes an open key only between A, B and candidates). So an id whose live count
  becomes zero is not eligible for the rest of the pass; the test of 2.2 is the live count.

2.2 Selection (replaces P1 2.3 item 2; item 1 stays: the key (A, B) is open and X has a nonzero sign).
With u, q as in P1 2.3 item 2 (same arithmetic; an intermediate that is not finite or q not positive: next
edge):
- Range. lo = min(A_s, B_s) - lim and hi = max(A_s, B_s) + lim, each ONE binary64 operation (round to
  nearest), then ONE `nextafter` outward (lo down, hi up).
- Find the inclusive range lo <= W_s <= hi in the sorted B0 with a lower and an upper bound search, and visit
  only that slice, in the order of B0 (the ids before the slice are not visited). For each visited id,
  in this order: charge (2.5); skip it if W is A, B or C; skip it if open(W) = 0 now; compute t and delta(W)
  as P1 2.3 item 2 (t in binary64 and kept fixed; delta with `Exact`, the interval `Multiply` and `Add`,
  `Difference`, `NormBound`; each intermediate and each interval end finite, else skip); W is a CANDIDATE
  when 0 < t < 1 and delta(W) <= lim.
- The EIGHTH candidate ends the visit at once: the edge is rejected as `long`. (So at most 7 candidates are
  stored. Only this list has a fixed size; B0 and the live counts grow with the input vertices.)
- No candidate: next edge; nothing is counted.
- Sort the candidates by (t, id). Two candidates with the same t: the edge is rejected as `tie`.
- Else the chain is A = V_0, the candidates in this order, B = V_n (2 <= n <= 8).
`FindWitnessChain` with its walk, the status `DeadEnd` and the counter `reject_dead_end` go away.

2.3 Fan (P1 2.3 item 3). The checks in THIS order; the first failure rejects the fan and X stays unchanged:
- `fan_key` (as P1);
- NEW `chain_key`: each chain key (V_i, V_i+1), i = 0 .. n-1, has at most ONE owner in the working list;
  and when it has one, the CURRENT chord of that owner has a local edge in the order (V_i+1, V_i) (the
  reverse of the child) and has NO local edge in the order (V_i, V_i+1) (a degenerate owner can have both:
  reject it);
- `not_finite`, `orientation`, `projection` (as P1).

2.4 Bound and commit. Count the committed fans of the pass. When a fan has passed ALL checks of 2.3 and the
count is already n_in (the number of input chords): do not admit and do not commit this fan; set `cap = 1`;
end the pass and return the current list. Reaching n_in alone does not set `cap`. Else commit as P1 2.3 item
4 and as the code of P1 does it: remove the incidences of X, then insert the incidences of each child at
its ACTUAL index of the working list (child 0 at the index of X, the others at their appended indices). The
net result: (A, B) goes away; each chain key gets the child as one more owner (from 0 to 1, or from 1 to
2); each key (V_i, C) of an inner V_i goes from 0 to 2; (C, A) and (B, C) keep their counts, and the owner
id of X is replaced by the id of child 0 and of child n-1 at EACH count (not only at count 1). Update
open(W) only from the net count changes of the unordered keys. The loop order of P1 2.3 stays (ascending index; after a commit the
same index again from k = 0; appended children are inspected later).

2.5 Work and storage. Charge ONE unit of work (the static `AtlasOps::Work(budget, 1)` of P1) before: each
input chord that is put into the maps; each distinct id that is put into B0; each local edge that the pass
inspects; each id of B0 that 2.2 visits in the inclusive range, BEFORE each skip test (the id that becomes
the eighth candidate included; the ids after it are not visited and not charged). For a fan: `fan_key` and
`chain_key` first, with no charge; then the children in ascending i with one unit immediately before the
`not_finite` check of that child, stop at the first failure; then `orientation` and `projection` with no
charge (as P1 2.5). The sort of B0 and the sort of the candidates have no charge of their own.
Storage: as P1 2.5, and each admission also names B0 (id and coordinate), the live counts, the candidate
scratch (fixed size) and the fan scratch. The final capacity charge of the list and the restore of
`caller_bytes` at the call site (AR:2946-2953) do not change. A refused admission is a failure of the query;
there is NO fall-back to the list that is not closed, also when a group of the closed list is `invalid`.

2.6 Row (print-only, both switches on, the isolation of P1 2.6):
`NATIVE_SURFACE_WITNESS_CLOSED chords_in= chords_out= splits= children= chain2= chain3= chain4= chain5_8=
reject_long= reject_tie= reject_fan_key= reject_chain_key= reject_not_finite= reject_orientation=
reject_projection= cap= left_contacts= origin=<as P1>`.
One reject is counted for an inspected edge at its FIRST failed check, in the order `long`, `tie`, `fan_key`,
`chain_key`, `not_finite`, `orientation`, `projection`. `cap` is 0 or 1.
`left_contacts` (replaces `left_closable`): on the FINAL list, the number of pairs (open key (A, B) with its
owner X; id W) with: W is not a corner of X; open(W) > 0; W passes the foot test of 2.2 (finite, 0 < t < 1,
delta <= lim). An open key is UNORDERED here: use the smallest local edge index of X that has the key, and
make the foot test in the direction of that local edge. Each pair (key, W) counts one time. The sign of X, the count of candidates and the fan checks do not
matter. The scan is outside the pure function, runs only for this row, reads only, and never calls `Work`,
`Storage` or a time check of the live budget (it may scan all ids; it need not use B0). A failed scan prints
`left_contacts=-1` with the other fields kept; a row that cannot be made prints
`NATIVE_SURFACE_TRACE_INCOMPLETE site=closed`.
WH: the counters of `witness::Counters` follow the row (`reject_dead_end` goes away; `reject_tie`,
`reject_chain_key` and `cap` come). `witness::Close` keeps its signature.

## 3. Why the proofs stay valid

P1 section 3 holds word for word. It needs of a chain only: fixed binary64 parameters that are strictly
increasing in (0, 1) (2.2: sorted, no tie); a certified delta <= lim for each inner vertex (2.2); the checks
`not_finite` and `orientation` (2.3). It does not use that a chain edge is a key of another chord. The
`chain_key` check and the cap only prevent commits. A chord of an `invalid` or `trimmed` group stays in the
ordinary distance path as today.

## 4. Hidden cases (G1 does not change)

Change the hidden cases of P1 section 4 in the existing unit-test files.
(a) Direct cases of the pure function (trace off), each with the asserted closed list, origin indices,
errors and counters.
- Kept, with the expectations of rule P: a T-junction of depth 1 between two "facets" and one inside a facet;
  a fully quartered edge; a mixed chain (A - M - Q - B); chains on two edges of one chord; a far false
  neighbor (farther than lim: no split); a near false neighbor (within lim, not on the edge: split, with the
  offset and the error asserted); an inner vertex whose key to C exists (`fan_key`); a child that would be
  inverted and one with zero area (no split); a parent with sign 0 (no split); a key with three chords (not
  open: no split); an input without a contact (the output list is bit-identical to the input).
- Changed: 7 inner vertices (split, 8 children) and 8 inner vertices (`long`, no split).
- Removed: the dead end with a later near vertex.
- New: a half seam with only the key (A, M) (split into 2 children; the key (M, B) is open after it); a half
  seam with only the key (M, B); a corner contact (a chord has W as a corner and no key along A-B: split); the
  live count: a full seam with both halves whose middle vertex M is in B0; the pass closes both halves; then a
  second chord is inspected that has M within its range and within lim: assert that this visit is charged and
  that M is skipped because open(M) = 0; two vertices with the same t (`tie`, no split); a chain key that has
  two owners (`chain_key`); a chain key whose one owner has the same direction as the child (for example the
  chord (A, W, D) as the sole owner of the key (A, W) when the fan would add A -> W; leave the key (W, C)
  absent so that `chain_key` is the first failed check); the cap: at resolution 1, three translated copies of the chord
  [(0,0), (2,0), (0,2)] in the plane z = 0, in this order, at the translations (0,0), (10,0), (0,10); then one
  chord through the three translated AB midpoints and one chord through the three translated CA midpoints
  (n_in = 5; each copy can split on AB and then on CA): assert five commits, ten returned chords and `cap=1`
  when the sixth fan has passed its checks, and the origin list 0,1,2,3,4,0,0,1,1,2 (a chord and its medial triangle do NOT give this: `fan_key`
  rejects the second fan); the range end: a vertex of B0 whose coordinate on s is exactly the range end AFTER
  the `nextafter`: assert the charged visit, whatever its foot test gives.
(b) The error rule of P1 4(b) on (a) cases with a known analytic surface.
(c) The public-query cases of P1 4(c) (improvement; lower path with a complete and with a cropped witness;
eviction), with the expectations of rule P.

## 5. The work, the gates and the labels

A new build number and receipt folders under `R/night_20261115/FP2_P2/`. Build `SU2_CFD` and
`UnitTests/test_driver`, no new warning; freeze and pin this binary pair. One owned heavy job; the host
preflight of R1 before each launch. The source base of the build provenance is `dec45f8a71` and the earlier
build is build670 (the build helper of P1 names `3228ca0e1e` and build668: change it). The references of
G1, of the fixtures, of build664 and of the shared checks stay those that these comparisons already use.
Runners, collector, aggregator, their self-checks and the report helper: copy those of FP2-P1 to `FP2_P2/`
and change them (R3: test code). The parser must read the row of 2.6 (the collector of P1 asks for
`reject_dead_end` and `left_closable`). Keep the marker handling of P1 (the marker line of P1 2.1 can stand
inside a stdout row: keep the raw logs, keep stderr apart where the launcher can, and use your verified
recovery for merged logs). The rules of P1 section 5 for the two switches, the checkpoint, the campaign, the
watchdogs and the shared checks hold.

Step 0. One R4 source check of this text and of the parts of P1 that it keeps.

Step 1, the default path (closure `0`) and the new cases: the five parts of P1 step 1, with the hidden cases
of section 4 of THIS text. The N1 trial of part 3 (closure `0`, trace `0`, option T on with `1E-4`, the lab
option of FP2-M2 on, M1 and both exporters off) is also the CONTROL of gate D: keep its `FP2_M2 STAGE` rows.
Label: "FP2-P2: fixtures passed; default path not yet proven", or "... fixtures FAILED ...", or
"... fixtures OPEN ...".

Step 2, closure `1` (lab, observation), in THIS order:
1. D1: N1 lab trial 1 (trace `0`, the settings of P1 step 2.1). It is trial 1 of the N1 pair of the campaign.
   The control and D1 have the same settings (`ADAP_HAUSD= 1E-4`, `SU2_NATIVE_FP2_M2=1`,
   `SU2_NATIVE_GEOMETRY_WORK_TRACE=0`, `SU2_NATIVE_FP2_M1=0`, `SU2_NATIVE_FP2_M2_TABLE=0`,
   `SU2_NATIVE_FP2_M3_FACETS=0`); only `SU2_NATIVE_CONFORMING_WITNESS` differs (`0` / `1`).
   Gate D: D1 and the control both qualified (use the selected qualified receipts of this binary pair, with
   your verified recovery of a merged marker line where it is necessary); each has exactly the ten stage
   keys (rank 0, pass 1, bin 0..4, direct_eligible 0..1), each one time, with a complete observer receipt;
   and the integer sum of `classification_work` over the ten rows of D1 is BELOW that of the control.
   (build670: control 463,538,756; rule C 685,848,058.)
   Report beside it, no gate: elapsed, step-C and Bound seconds of both; `traversal_work`,
   `scope_build_work`.
   Dispositions. SEAL D1 before D2 is launched. D1 passed: go on. A confirmed numeric failure of gate D: R5
   (below). D1 ends OPEN (a second wrapper-only timeout), or its measurement is not available: D2 and item 3
   are "not evaluated". A measurement that is still wrong or missing after the R3 / R6 work, with a run that
   was not interrupted: the measurement is FAILED (test code; no production revert), and D2 and item 3 are
   "not evaluated".
2. D2 (only after D1 passed): the first-collapse check with closure `1`, complete, with its unchanged
   acceptance (work limit 108,459,426), its inherited deadline, and the resource units verified as in P1.
   Judge it on the complete receipt. A hard failure that was seen stays a failure. Only a verified
   wrapper-only interruption with no hard failure seen is OPEN; it stops item 3. A hard check that was not
   evaluated stops item 3. A difference of a historical decision-identity value alone is a "missed
   comparison": OPEN for the user; it does not stop item 3 (and, later, step 3) when each hard check of D2
   passed. On a verified wrapper-only interruption: a check that needs rank rows that are missing is "not
   evaluated"; do not read a work or refinement failure from the booleans of an incomplete run (the
   inherited evaluator sets `upper_work_pass` and `upper_refinements_pass` false when rank rows are missing).
   Keep each hard failure that the available evidence shows: a partial work sum above the limit, a seen
   refinement, a resource breach, the inherited 1,500-second first-collapse deadline. Without one, seal D2
   OPEN with no promotion. Apply the complete-run resource seal only when its evidence exists.
   SEAL the decision of D2 before item 3 is launched. Pin the decisions of D1 and D2 and their selected
   receipts to the current binary pair; put both in the lab checkpoint; the launcher of item 3 and the
   launcher of the run S verify both promotion records before they start. (The stage driver of P1 puts all
   trials and checks in one queue at the start: change that.)
3. Only when the sealed decisions of D1 and D2 allow it: N1 trial 2, N6P and N6 (two trials each), the three exact pair
   comparisons; the report of P1 step 2.2; ANATOMY as P1 step 2.3, with the counters of 2.6 in the aggregator
   (sum each counter except `left_contacts` of each valid row; sum `left_contacts` only where it is not
   negative; `cap` as a count of rows with `cap=1`) and with one more table over the completed miss / bypass
   forwards: for each of the five length bins with `direct_eligible=0`, and for the ONE pooled cohort
   `direct_eligible=1`, the `PLANES` rows and the integer sums of their raw fields `groups`, `accepted` (it
   includes a group that a trim made valid), `invalid`, `boundary_trimmed`, beside those of FP2-M2 and
   FP2-P1 (for a cell of these two sealed runs: zero rows give the sum zero; else take the nearest integer
   to stored mean x row count, and check that this integer divided by the row count, in the binary64
   arithmetic of the aggregator, gives the stored mean again and that no other integer does; a cell that is
   not available or not consistent is a report finding under R3 / R6). Contact and footprint counts exist for the traced N1 only; write that for N6 and
   N6P; the other four shared checks with closure `1` as P1 step 2.4.
A confirmed failure of gate D or of a hard check of D2: R5 as P1 reads it (keep the first number; one
diagnosis of at most 60 minutes; a confirmed transcription or wiring fault against this text: one correction,
a new binary pair, the affected checks again). If the failure stays: the step is FAILED, items after it are
"not evaluated", and the commit stays with the switch off (the closure-`1` rule of P1).
Label by the common label rule of P1: "FP2-P2: lab passed; default path not yet proven", or "... lab OPEN
...", or "... lab FAILED ...".

Step 3, closure `1`, trace `0`: one run S with its isolated restart, as P1 step 3, with the start condition of
P1 (six qualified trials, three equal pairs, no required part of an earlier step FAILED). Report it beside
the sealed run S of build664; FP2-P1 has no run S: write "not evaluated" for it. Label "FP2-P2: S
passed / FAILED / OPEN; default path not yet proven". After "lab FAILED" there is NO step 3, NO label of
step 3 and no later checkpoint label: the last label is that of step 2, and the report writes the parts of
step 3 as "not evaluated". (This replaces the sentence of P1 step 3 that asks for an "S OPEN" label then.)

After the last label: update the progress file, `HANDOFF_Codex.md`, `HowToProceed.md` and
`Handbook/ADAPTATION_TIMINGS.md`; write the report `R/night_20261115/FP2_P2/reports/FP2_P2_<build>.md` and
`.json` (the status of each part; the list "not evaluated"; the production diff; gate D with both sums; the
tables of steps 2 and 3 beside build664 and FP2-P1; no recommendation). Then STOP and wait for the next user
order. The rules R1, R3, R5 (as P1 reads it), R6, R7, R8, R9, R10 (with the switch off) and R13 hold.

## 6. Not in this order

The switch on by default, a config option for it, a new reference, a change of a gate or of the tolerance.
A change of `BuildPlanes`, of `FinitePlaneReference`, of the coverage test, of the lift, of the facet leaves,
of the witness selection (the crop), of the local model path. A second build of the witness that is not
closed. A change of the checks `orientation` and `projection` or of the limit of 8 chain edges. G2, GQ, G3,
G5 and the full no-BL workflow with the switch off, the 18 FP1 cohorts, and a second run S.
